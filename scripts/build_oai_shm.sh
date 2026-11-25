#!/usr/bin/env bash
# Internal requirements.sh helper: native, simulation-only OAI v3.0.0 + SHM.
# No sudo, /opt installation, SDR dependency installation, or runtime launch.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
die() { echo "ERROR: $*" >&2; exit 1; }
[ $# -le 1 ] || die "usage: $0 [CONDA_PREFIX]"
PREFIX="${1:-${CONDA_PREFIX:-$HOME/miniforge3/envs/5ghoul23}}"
[ -d "$PREFIX/lib" ] || die "conda environment not found: $PREFIX"
PREFIX="$(cd "$PREFIX" && pwd -P)"

DEPS="$ROOT/.deps"
SOURCE="$DEPS/oai-src"
BUILD="$SOURCE/build"
DIST="$DEPS/oai-shm/bin"
LOG_ROOT="$ROOT/logs/install/oai"
OAI_HEAD=6f89078b409e671fa566782f67aae9de52fd55d8
SIMDE_HEAD=ef2f8cc0c5e115ae9139096ee45a0b02d2563f7e
ASN1_HEAD=940dd5fa9f3917913fd487b13dfddfacd0ded06e

# OAI and its local ASN1 ExternalProject must never inherit conda compilers,
# include/lib search paths, preload shims, or another project's cache settings.
unset CC CXX CPP CFLAGS CXXFLAGS CPPFLAGS LDFLAGS LIBS LD_LIBRARY_PATH LD_PRELOAD
unset AS AR RANLIB LD NM STRIP OBJCOPY OBJDUMP SIZE READELF
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH COMPILER_PATH GCC_EXEC_PREFIX
unset CMAKE_PREFIX_PATH CMAKE_LIBRARY_PATH CMAKE_INCLUDE_PATH CMAKE_TOOLCHAIN_FILE
unset CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
unset PKG_CONFIG_PATH PKG_CONFIG_LIBDIR PKG_CONFIG_SYSROOT_DIR CONFIG_SITE MAKE MAKEFLAGS MFLAGS
unset CONDA_BUILD_SYSROOT CMAKE_GENERATOR CMAKE_GENERATOR_PLATFORM CMAKE_GENERATOR_TOOLSET
export PATH=/usr/bin:/bin CC=/usr/bin/gcc-15 CXX=/usr/bin/g++-15 LC_ALL=C

# Apt prerequisites (requirements.sh installs these, not this helper):
# build-essential gcc-15 g++-15 cmake ninja-build git ca-certificates pkg-config
# autoconf automake libtool bison flex libconfig-dev libsctp-dev libssl-dev
# zlib1g-dev xxd patchelf binutils util-linux. ccache is optional.
for tool in git cmake ninja pkg-config gcc-15 g++-15 autoreconf autoconf automake \
  libtoolize bison flex make ar nm objdump readelf xxd patchelf ldd flock; do
  [ -x "/usr/bin/$tool" ] || die "missing native prerequisite: /usr/bin/$tool"
done
[ "$(/usr/bin/gcc-15 -dumpversion)" = 15 ] || die "native GCC 15 required"
[ "$(/usr/bin/g++-15 -dumpversion)" = 15 ] || die "native G++ 15 required"
pkg-config --exists libconfig openssl || die "install libconfig-dev and libssl-dev"
[ -f /usr/include/netinet/sctp.h ] || die "install libsctp-dev"
[ -f /usr/include/zlib.h ] || die "install zlib1g-dev"

for dir in "$DEPS" "$DEPS/oai-shm" "$DIST" "$SOURCE" "$BUILD"; do
  [ ! -L "$dir" ] || die "refusing symlinked owned path: $dir"
  [ ! -e "$dir" ] || [ -d "$dir" ] || die "not a directory: $dir"
done
mkdir -p "$DEPS" "$LOG_ROOT"
exec 9>"$DEPS/.oai-install.lock"
flock -n 9 || die "another OAI installer is running"
LOG="$(mktemp -d "$LOG_ROOT/run-$(date -u +%Y%m%dT%H%M%SZ)-XXXXXX")"
exec > >(tee "$LOG/install.log") 2>&1
STAGE=
cleanup() {
  local status=$?
  trap - EXIT
  [ -z "$STAGE" ] || rm -rf -- "$STAGE"
  printf '%s\n' "$status" > "$LOG/exit-status"
  exit "$status"
}
trap cleanup EXIT
printf 'OAI source=%s\nbuild=%s\ndist=%s\nconda runtime=%s\n' "$SOURCE" "$BUILD" "$DIST" "$PREFIX"

if [ ! -e "$SOURCE" ]; then
  git clone --depth 1 --branch v3.0.0 \
    https://gitlab.eurecom.fr/oai/openairinterface5g.git "$SOURCE"
fi
[ -e "$SOURCE/.git" ] || die "not an OAI git checkout: $SOURCE"
[ "$(git -C "$SOURCE" rev-parse --show-toplevel)" = "$SOURCE" ] || die "expected OAI checkout root"
[ "$(git -C "$SOURCE" rev-parse HEAD)" = "$OAI_HEAD" ] || die "wrong OAI HEAD; preserving checkout"

# Leave unrelated untracked content alone, but never build an ambiguous source.
while IFS= read -r -d '' path; do
  case "$path" in
    build/*|shm_interface|openair1/simde|openair1/PHY/simde/*) ;;
    *) die "unexpected untracked OAI source: $path" ;;
  esac
done < <(git -C "$SOURCE" ls-files --others --exclude-standard -z)
"$ROOT/scripts/apply_oai_shm_patch.sh" "$SOURCE"

# The validated header tree is local to this checkout. No /usr/include copy.
SIMDE="$SOURCE/openair1/PHY/simde"
[ ! -L "$SIMDE" ] || die "refusing symlinked SIMDe checkout"
if [ ! -e "$SIMDE" ]; then
  git init "$SIMDE"
  git -C "$SIMDE" remote add origin https://github.com/simd-everywhere/simde-no-tests.git
  git -C "$SIMDE" fetch --depth 1 origin "$SIMDE_HEAD"
  git -C "$SIMDE" checkout --detach FETCH_HEAD
fi
[ -e "$SIMDE/.git" ] || die "SIMDe is not a git checkout"
[ "$(git -C "$SIMDE" rev-parse --show-toplevel)" = "$SIMDE" ] || die "expected SIMDe checkout root"
[ "$(git -C "$SIMDE" rev-parse HEAD)" = "$SIMDE_HEAD" ] || die "wrong SIMDe HEAD; preserving checkout"
[ -z "$(git -C "$SIMDE" status --porcelain --untracked-files=all)" ] || die "dirty SIMDe checkout"
[ -f "$SIMDE/x86/avx2.h" ] || die "SIMDe header tree incomplete"
if [ -e "$SOURCE/openair1/simde" ] && [ ! -L "$SOURCE/openair1/simde" ]; then
  die "openair1/simde is not a symlink; refusing to replace it"
fi
if [ ! -L "$SOURCE/openair1/simde" ] || [ "$(readlink "$SOURCE/openair1/simde")" != PHY/simde ]; then
  ln -sfnT PHY/simde "$SOURCE/openair1/simde"
fi

# Refuse a foreign build rather than deleting it or reusing incompatible flags.
STAMP="native-v1 $OAI_HEAD $SIMDE_HEAD $ASN1_HEAD /usr/bin/gcc-15 /usr/bin/g++-15"
if [ -d "$BUILD" ] && [ -n "$(find "$BUILD" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
  [ -f "$BUILD/.5ghoul-oai-build" ] && [ "$(cat "$BUILD/.5ghoul-oai-build")" = "$STAMP" ] ||
    die "unowned or incompatible OAI build tree; preserving $BUILD"
else
  mkdir -p "$BUILD"
  printf '%s\n' "$STAMP" > "$BUILD/.5ghoul-oai-build"
fi
export CPM_SOURCE_CACHE="$BUILD/_cpm-cache" CCACHE_DIR="$BUILD/_ccache"
export CCACHE_BASEDIR="$SOURCE" CCACHE_COMPILERCHECK=content
unset CPM_USE_LOCAL_PACKAGES CPM_DOWNLOAD_ALL CCACHE_PREFIX CCACHE_PATH CCACHE_CONFIGPATH
mkdir -p "$CPM_SOURCE_CACHE" "$CCACHE_DIR"

# Deliberately leave ASN1C_EXEC empty: the upstream pinned ExternalProject then
# builds/installs ASN1c inside BUILD, even if /opt/asn1c or an apt compiler exists.
# Autoreconf/bison/flex/libtool/make are checked above. ExternalProject stamps
# preserve incremental builds on subsequent invocations.
/usr/bin/cmake -S "$SOURCE" -B "$BUILD" -G Ninja \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-15 -DCMAKE_CXX_COMPILER=/usr/bin/g++-15 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DASN1C_EXEC:FILEPATH= -DAUTO_DOWNLOAD_ASN1C=ON \
  -DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config \
  -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF \
  -DENABLE_TESTS=OFF -DENABLE_PHYSIM_TESTS=OFF -DT_TRACER=ON \
  -DENABLE_LDPC_CUDA=OFF -DENABLE_CHANNEL_SIM_CUDA=OFF
/usr/bin/ninja -C "$BUILD" -j8 nr-softmodem nr-uesoftmodem params_libconfig rfsimulator
[ "$(git -C "$BUILD/asn1c_gen-prefix/src/asn1c_gen" rev-parse HEAD)" = "$ASN1_HEAD" ] ||
  die "unexpected ASN1c source version"
for option in -gen-APER -no-gen-UPER -no-gen-JER -no-gen-BER -no-gen-OER; do
  status=0
  "$BUILD/bin/asn1c" "$option" > "$LOG/asn1c${option}.txt" 2>&1 || status=$?
  [ "$status" = 1 ] || die "local ASN1c does not support $option (exit $status)"
done

# Step 4 gates inspect the genuine, unstripped outputs before touching DIST.
for mode in gnb ue; do
  binary="$BUILD/nr-softmodem"
  [ "$mode" != ue ] || binary="$BUILD/nr-uesoftmodem"
  nm "$binary" > "$LOG/$mode.nm"
  nm --undefined-only "$binary" > "$LOG/$mode.undefined.nm"
  # Inspect the entire text, retaining the actual call instructions rather
  # than hundreds of megabytes of unrelated disassembly on every rerun.
  objdump -d --no-show-raw-insn "$binary" |
    awk '/[[:space:]]call[q]?[[:space:]].*<shm_init>/ {print}' > "$LOG/$mode.objdump"
done
gnb_symbols="$(awk '$NF ~ /^shm_/ {n++} END {print n+0}' "$LOG/gnb.nm")"
ue_symbols="$(awk '$NF ~ /^shm_/ {n++} END {print n+0}' "$LOG/ue.nm")"
ue_undefined="$(awk '$NF ~ /^shm_/ {n++} END {print n+0}' "$LOG/ue.undefined.nm")"
gnb_calls="$(awk '/[[:space:]]call[q]?[[:space:]].*<shm_init>/ {n++} END {print n+0}' "$LOG/gnb.objdump")"
ue_calls="$(awk '/[[:space:]]call[q]?[[:space:]].*<shm_init>/ {n++} END {print n+0}' "$LOG/ue.objdump")"
printf 'gNB SHM symbols=%s\nUE SHM symbols=%s\nUE undefined SHM symbols=%s\ngNB actual shm_init calls=%s\nUE actual shm_init calls=%s\n' \
  "$gnb_symbols" "$ue_symbols" "$ue_undefined" "$gnb_calls" "$ue_calls" | tee "$LOG/step4-gates.txt"
[ "$gnb_symbols" = 12 ] && [ "$ue_symbols" = 0 ] && [ "$ue_undefined" = 0 ] && \
  [ "$gnb_calls" = 1 ] && [ "$ue_calls" = 0 ] || die "Step 4 SHM gate failed; not installing"
for symbol in shm_init shm_wait shm_mq_recv shm_mq_thread_handler; do
  awk -v name="$symbol" '$2 == "T" && $3 == name {found=1} END {exit !found}' "$LOG/gnb.nm" ||
    die "gNB API missing: $symbol"
done
printf 'PASS: Step 4 SHM gates\n' | tee -a "$LOG/step4-gates.txt"

# OAI resets CMAKE_INSTALL_RPATH itself and dlopen()s bare module names.
# Patch only the install copies to a relocatable $ORIGIN RUNPATH, so binaries
# and all five modules load from their own bin directory, not the build tree.
mkdir -p "$DEPS/oai-shm"
STAGE="$(mktemp -d "$DEPS/oai-shm/.install-XXXXXX")"
FILES=(nr-softmodem nr-uesoftmodem libdfts.so libldpc.so libldpc_orig.so libparams_libconfig.so librfsimulator.so)
for filename in "${FILES[@]}"; do
  [ -f "$BUILD/$filename" ] || die "missing build artifact: $filename"
  install -m 0755 "$BUILD/$filename" "$STAGE/$filename"
  patchelf --set-rpath '$ORIGIN' "$STAGE/$filename"
  readelf -d "$STAGE/$filename" > "$LOG/$filename.dynamic.txt"
  ldd "$STAGE/$filename" > "$LOG/$filename.ldd.txt" 2>&1 || die "cannot resolve $filename libraries"
  if grep -q 'not found' "$LOG/$filename.ldd.txt"; then
    die "missing libraries for $filename; not installing"
  fi
done
mkdir -p "$DIST"
for filename in "${FILES[@]}"; do
  [ ! -d "$DIST/$filename" ] || die "refusing to replace directory: $DIST/$filename"
done
for filename in "${FILES[@]}"; do
  mv -fT "$STAGE/$filename" "$DIST/$filename"
done
rmdir "$STAGE"
STAGE=
printf 'Installed gated native OAI v3.0.0 to %s\nLogs: %s\n' "$DIST" "$LOG"
