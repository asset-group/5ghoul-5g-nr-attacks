#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Configure 5Ghoul against a conda-forge clang toolchain.
#
# Usage:
#   ./run-configure.sh                 # env 5ghoul23, clang 23 (proven build)
#   conda env create -f environment.yml  # single default profile
#
# Everything that made configure work on Ubuntu 26.04 lives here on purpose:
# conda clang has a sysroot that excludes /usr/include, lld has no implicit
# $CONDA_PREFIX/lib search path, and the libstdc++ headers must come from the
# conda GCC 16.2.0 install, selected explicitly with --gcc-install-dir below
# (-include cstdint supplies declarations no longer transitively included).
# ---------------------------------------------------------------------------
set -eo pipefail

CLANG_MAJOR="${1:-23}"
ENV_NAME="${2:-5ghoul23}"
CLONE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$HOME/miniforge3/etc/profile.d/conda.sh"
conda activate "$ENV_NAME" || { echo "FATAL: cannot activate $ENV_NAME"; exit 1; }

# --- conda-forge packaging gap: the boost_system component is missing ------
# conda-forge's split boost packaging (recipe "boost-split") ships a cmake
# package dir for every component except boost_system, although Boost 1.90's
# BoostConfig.cmake still requests it for `find_package(Boost COMPONENTS
# system ...)`, which both 5Ghoul and vendored wireshark do.  The component is
# header-only since Boost 1.69, so an INTERFACE target is the whole content of
# the missing file.  Written here (not hand-edited into the env) so the fix is
# reproducible from the repo.
repair_boost_system_component() {
  local ver=1.90.0 dir headers
  dir="$CONDA_PREFIX/lib/cmake/boost_system-$ver"
  headers="$CONDA_PREFIX/lib/cmake/boost_headers-$ver"
  [ -f "$headers/boost_headers-config-version.cmake" ] || {
    echo "FATAL: environment.yml requires Boost headers $ver" >&2; return 1;
  }
  if [ ! -f "$dir/boost_system-config.cmake" ]; then
    mkdir -p "$dir"
    cat > "$dir/boost_system-config.cmake" <<EOF
# Generated for conda-forge's split boost $ver packaging, which omits the
# header-only boost_system component that BoostConfig.cmake still requests.
if(TARGET Boost::system)
  return()
endif()
mark_as_advanced(boost_system_DIR)
find_package(boost_headers $ver EXACT CONFIG REQUIRED)
add_library(Boost::system INTERFACE IMPORTED)
set_target_properties(Boost::system PROPERTIES
  INTERFACE_LINK_LIBRARIES Boost::headers
)
set(boost_system_FOUND TRUE)
EOF
    cp "$headers/boost_headers-config-version.cmake" \
       "$dir/boost_system-config-version.cmake"
    echo "[env repair] wrote missing boost_system component config for $ver"
  fi
}
repair_boost_system_component

# Folly links the unversioned development name, absent in some conda exports.
if [ ! -e "$CONDA_PREFIX/lib/liblzma.so" ]; then
  [ -e "$CONDA_PREFIX/lib/liblzma.so.5" ] || {
    echo "FATAL: missing $CONDA_PREFIX/lib/liblzma.so.5" >&2; exit 1;
  }
  ln -sfn liblzma.so.5 "$CONDA_PREFIX/lib/liblzma.so"
fi
GCC_INSTALL_DIR="$CONDA_PREFIX/lib/gcc/x86_64-conda-linux-gnu/16.2.0"
[ -d "$GCC_INSTALL_DIR" ] || {
  echo "FATAL: environment.yml requires GCC headers at $GCC_INSTALL_DIR" >&2
  exit 1
}

export CC="clang-${CLANG_MAJOR}"
export CXX="clang++-${CLANG_MAJOR}"

# -idirafter /usr/include: conda clang is built with --sysroot, so system dev
# headers are invisible; appended, so sysroot glibc headers still win.
# -include cstdint: GCC 16 no longer leaks fixed-width integer declarations.
# -Wno-format-truncation: clang 23 diagnoses snprintf() calls that provably
# truncate (libs/wireshark/epan/dissectors has several, e.g.
# packet-5co-legacy.c:923), and vendored wireshark compiles with -Werror.
# Disabled globally rather than patching ~1700 vendored dissectors.
COMMON_FLAGS="-march=nocona -mtune=haswell -ftree-vectorize -fPIC \
-fstack-protector-strong -fno-plt -O2 -ffunction-sections -pipe \
-Wno-format-truncation \
-isystem $CONDA_PREFIX/include -idirafter /usr/include \
--gcc-install-dir=$GCC_INSTALL_DIR"
# clang 20+ promotes several C diagnostics to errors by default (not via
# -Werror).  Vendored wireshark trips them: its epan plugins register 3-arg
# dissectors against the 4-arg dissector_t, e.g.
# libs/wireshark/plugins/epan/h4bcm/packet-btbrlmp.c:97.  Demote them back to
# warnings for C only; the calls are ABI-safe on x86-64 (the extra argument
# register is simply ignored) and these are optional protocol plugins.
export CFLAGS="$COMMON_FLAGS \
-Wno-error=incompatible-function-pointer-types \
-Wno-error=implicit-function-declaration \
-Wno-error=implicit-int \
-Wno-error=int-conversion"
# -I libs/pybind11/include: conda's clang driver ships clang++.cfg containing
# "-isystem <CFGDIR>/../include", i.e. every compile implicitly searches
# $CONDA_PREFIX/include *before* any -isystem given on the command line.  CMake
# marks pybind11's own include as SYSTEM (-isystem), so conda's pybind11 3.1.0
# won over the vendored 2.11.0.dev1 and died on "PYTHON < 3.9 IS UNSUPPORTED"
# against the bundled CPython 3.8.16.  Reordering -isystem does not help; -I
# directories are searched before all system directories, and -I does win
# (verified with clang -H).  The vendored tree is complete.
#
# -DGLOG_USE_GLOG_EXPORT: glog 0.7.x makes consumers opt in to the export
# header; without it <glog/logging.h> and <glog/flags.h> both stop with
# "#error <glog/...> was not included correctly" (hit first in
# conda folly's Executor.cpp).  Defined for every C++ TU so folly and its
# consumers agree.
export CXXFLAGS="$COMMON_FLAGS -fvisibility-inlines-hidden \
-DGLOG_USE_GLOG_EXPORT \
-I$CLONE/libs/pybind11/include \
-include cstdint"
# -L$CONDA_PREFIX/lib: lld has no implicit env lib dir.
# -Wl,-rpath-link,...: resolve split GLVND/system libs at link time.
export LDFLAGS="-L$CONDA_PREFIX/lib \
-Wl,-rpath-link,$CONDA_PREFIX/x86_64-conda-linux-gnu/sysroot/usr/lib \
-Wl,-rpath-link,/usr/lib/x86_64-linux-gnu"

mkdir -p "$CLONE/logs"
LOG="$CLONE/logs/configure-clang${CLANG_MAJOR}.log"
{
  echo "=== configure start $(date) ==="
  echo "distro : $(lsb_release -ds 2>/dev/null)  kernel: $(uname -r)"
  echo "env    : $ENV_NAME  prefix: $CONDA_PREFIX"
  echo "clang  : $($CXX --version | head -1)"
  echo "lld    : $( (lld --version || ld.lld --version) 2>/dev/null | head -1)"
  echo "llvm-ar: $(command -v llvm-ar)  ($(llvm-ar --version 2>/dev/null | head -1))"
  echo "stdc++ : $(echo '#include <bits/c++config.h>
_GLIBCXX_RELEASE' | $CXX -std=c++20 -E -P -x c++ - 2>/dev/null | tail -1)"
} | tee "$LOG"

cd "$CLONE" || exit 1
# -DENABLE_LUA=OFF: conda-forge's lua ships only liblua.a, built without -fPIC,
# so lld refuses to link it into bin/libwireshark.so ("relocation R_X86_64_32
# cannot be used against local symbol").  Lua dissector support is not used by
# 5Ghoul, so the feature is switched off rather than rebuilding lua.
set +e
cmake -B build -S . -G Ninja \
      -DWDS_CLANG_MAJOR="$CLANG_MAJOR" \
      -DWIRESHARK_GUI=OFF \
      -DENABLE_RPATH_ORIGIN=TRUE \
      -DENABLE_WERROR=OFF \
      -DENABLE_LUA=OFF \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" \
      -DEIGEN3_INCLUDE_DIR="$CONDA_PREFIX/include/eigen3" \
      -DCMAKE_C_FLAGS="$CFLAGS" \
      -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
      -DPython3_EXECUTABLE="$CONDA_PREFIX/bin/python3" \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 2>&1 | tee -a "$LOG"
rc=${PIPESTATUS[0]}
echo "=== configure exit $rc $(date) ===" | tee -a "$LOG"
exit $rc
