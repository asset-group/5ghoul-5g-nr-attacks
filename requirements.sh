#!/usr/bin/env bash
# Install the complete Ubuntu 26.04 x86_64 simulation/build stack.
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
MINIFORGE=${MINIFORGE_ROOT:-$HOME/miniforge3}
PREFIX="$MINIFORGE/envs/5ghoul23"
DEPS="$ROOT/.deps"
LOG="$ROOT/logs/install"
die() { echo "requirements.sh: $*" >&2; exit 1; }
[[ $# == 0 ]] || die 'usage: ./requirements.sh (no legacy modes)'
[[ $(uname -m) == x86_64 ]] || die 'the pinned stack requires Linux x86_64'
source /etc/os-release
[[ $ID == ubuntu && $VERSION_ID == 26.04 ]] || die 'supported host: Ubuntu 26.04'
cd "$ROOT"
mkdir -p "$DEPS" "$LOG"
exec > >(tee "$LOG/requirements.log") 2>&1
trap 'echo "requirements.sh: failed at line $LINENO" >&2' ERR
df -h "$ROOT"

# Conda provides the 5Ghoul toolchain/libraries. Native development packages
# are required for Linux SCTP/core metadata and OAI's separate GCC 15 build.
SUDO=()
if ((EUID)); then SUDO=(sudo); fi
"${SUDO[@]}" apt-get update
"${SUDO[@]}" apt-get install -y --no-install-recommends \
  ca-certificates curl git sudo build-essential gcc-15 g++-15 cmake ninja-build \
  pkg-config autoconf automake libtool bison flex binutils patchelf \
  libconfig-dev libsctp-dev libtalloc-dev libyaml-dev libyaml-cpp-dev \
  libssl-dev libgnutls28-dev zlib1g-dev libreadline-dev libfftw3-dev \
  libmbedtls-dev libblas-dev liblapack-dev libgmp-dev libusb-1.0-0-dev \
  libx11-dev libxft-dev libxt-dev libasound2-dev libpulse-dev \
  libxss1 libnss3 libnspr4 libatk-bridge2.0-0 libxtst6 \
  kmod psmisc tcpdump iproute2 iptables util-linux xxd

if [[ ! -f "$MINIFORGE/etc/profile.d/conda.sh" ]]; then
  [[ ! -e "$MINIFORGE" ]] || die "incomplete Miniforge directory: $MINIFORGE"
  INSTALLER="$DEPS/Miniforge3-26.9.4-2-Linux-x86_64.sh"
  curl --fail --location --output "$INSTALLER" \
    https://github.com/conda-forge/miniforge/releases/download/26.9.4-2/Miniforge3-26.9.4-2-Linux-x86_64.sh
  printf '%s  %s\n' bbaf16b3897da24c4be50315934ba74848f617b4495982841d6a0a60e0edbbdf "$INSTALLER" | sha256sum --check
  bash "$INSTALLER" -b -p "$MINIFORGE"
fi

# Activation scripts can reference unset variables; strict mode resumes after
# activation. Never replace the canonical profile with an exported old env.
set +u
source "$MINIFORGE/etc/profile.d/conda.sh"
set -u
if [[ -d "$PREFIX/conda-meta" ]]; then
  conda env update --prefix "$PREFIX" --file "$ROOT/environment.yml" --prune
else
  conda env create --prefix "$PREFIX" --file "$ROOT/environment.yml" --yes
fi
set +u
conda activate "$PREFIX"
set -u
export LD_LIBRARY_PATH="$PREFIX/lib"
unset LD_PRELOAD
conda list --json > "$LOG/conda-packages.json"

# Boost.System is header-only; conda's split 1.90 package omits its component
# metadata. Reproduce the exact version semantics of the headers component.
BOOST_HEADERS="$PREFIX/lib/cmake/boost_headers-1.90.0"
BOOST_SYSTEM="$PREFIX/lib/cmake/boost_system-1.90.0"
[[ -f "$BOOST_HEADERS/boost_headers-config-version.cmake" ]] || die 'Boost headers 1.90.0 missing'
mkdir -p "$BOOST_SYSTEM"
if [[ ! -f "$BOOST_SYSTEM/boost_system-config.cmake" ]]; then
  cat > "$BOOST_SYSTEM/boost_system-config.cmake" <<'EOF'
if(TARGET Boost::system)
  return()
endif()
mark_as_advanced(boost_system_DIR)
find_package(boost_headers 1.90.0 EXACT CONFIG REQUIRED)
add_library(Boost::system INTERFACE IMPORTED)
set_target_properties(Boost::system PROPERTIES INTERFACE_LINK_LIBRARIES Boost::headers)
set(boost_system_FOUND TRUE)
EOF
fi
if [[ ! -f "$BOOST_SYSTEM/boost_system-config-version.cmake" ]]; then
  cp "$BOOST_HEADERS/boost_headers-config-version.cmake" "$BOOST_SYSTEM/boost_system-config-version.cmake"
fi
if [[ ! -e "$PREFIX/lib/liblzma.so" ]]; then
  [[ -e "$PREFIX/lib/liblzma.so.5" ]] || die 'liblzma.so.5 missing'
  ln -s liblzma.so.5 "$PREFIX/lib/liblzma.so"
fi

# The environment file installs modules/requirements.txt and PyMongo through
# its pip section. npm installs only developer quicktype and the real shell.
if [[ ! -x "$PREFIX/bin/quicktype" ]]; then
  "$PREFIX/bin/npm" install --global --prefix "$PREFIX" --no-audit --no-fund quicktype
fi
"$ROOT/scripts/install_mongosh.sh" "$PREFIX"

# Initialization and patching happen here, never during CMake configuration.
"$ROOT/scripts/apply_patches.sh" 3rd-party
"$ROOT/scripts/apply_patches.sh" ./libs
"$ROOT/scripts/apply_patches.sh" ./modules
"$ROOT/scripts/build_mongo_driver.sh" "$DEPS" "$PREFIX"
"$ROOT/scripts/build_open5gs.sh" "$DEPS" "$PREFIX"
"$ROOT/scripts/build_oai_shm.sh" "$PREFIX"
printf '\nInstalled env: %s\nOpen5GS: %s\nOAI dist: %s\nNext: ./build.sh\n' \
  "$PREFIX" "$ROOT/3rd-party/open5gs-core/build/tests/app/5gc" "$ROOT/.deps/oai-shm/bin"
