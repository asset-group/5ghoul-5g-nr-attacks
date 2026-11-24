#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Configure 5Ghoul against a conda-forge clang toolchain.
#
# Usage:
#   ./run-configure.sh                 # env 5ghoul23, clang 23 (proven build)
#   ./run-configure.sh 15 5ghoul       # env 5ghoul,   clang 15 (legacy, GCC 9 pins)
#
# Everything that made configure work on Ubuntu 26.04 lives here on purpose:
# conda clang has a sysroot that excludes /usr/include, lld has no implicit
# $CONDA_PREFIX/lib search path, and the libstdc++ headers must come from the
# conda GCC 16.2.0 install, selected explicitly with --gcc-install-dir below
# (GCC 9 <valarray> breaks under C++20, and GCC 13+ <format> clashes with the
# vendored fmt, which the stdcxx_compat.h include in CXXFLAGS works around).
# ---------------------------------------------------------------------------
set -o pipefail

CLANG_MAJOR="${1:-23}"
ENV_NAME="${2:-5ghoul23}"
CLONE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$HOME/miniforge3/etc/profile.d/conda.sh"
conda activate "$ENV_NAME" || { echo "FATAL: cannot activate $ENV_NAME"; exit 1; }
# scripts/apply_patches.sh calls sudo; the tree is user-owned, so shim it out.
export PATH="$HOME/.issue46-shim:$PATH"

# --- conda-forge packaging gap: the boost_system component is missing ------
# conda-forge's split boost packaging (recipe "boost-split") ships a cmake
# package dir for every component except boost_system, although Boost 1.90's
# BoostConfig.cmake still requests it for `find_package(Boost COMPONENTS
# system ...)`, which both 5Ghoul and vendored wireshark do.  The component is
# header-only since Boost 1.69, so an INTERFACE target is the whole content of
# the missing file.  Written here (not hand-edited into the env) so the fix is
# reproducible from the repo.
repair_boost_system_component() {
  local ver dir d
  # The component directories carry the full three-part version
  # (boost_headers-1.90.0) while boost/version.hpp only carries
  # BOOST_LIB_VERSION "1_90", i.e. 1.90.  Deriving from version.hpp therefore
  # never matched the boost_headers-* guard below, so this repair silently
  # no-opped and configure died in BoostConfig.cmake.  Take the version from
  # the Boost-<ver> package directory BoostConfig.cmake itself lives in, and
  # only fall back to version.hpp when that directory is absent.
  for d in "$CONDA_PREFIX"/lib/cmake/Boost-*/; do
    [ -d "$d" ] || continue
    ver="${d%/}"; ver="${ver##*/}"; ver="${ver#Boost-}"
    break
  done
  if [ -z "${ver:-}" ] && [ -f "$CONDA_PREFIX/include/boost/version.hpp" ]; then
    ver=$(sed -n 's/.*BOOST_LIB_VERSION "\(.*\)".*/\1/p' \
          "$CONDA_PREFIX/include/boost/version.hpp" | tr '_' '.')
  fi
  [ -n "${ver:-}" ] || return 0
  dir="$CONDA_PREFIX/lib/cmake/boost_system-$ver"
  if [ -d "$CONDA_PREFIX/lib/cmake/boost_headers-$ver" ] && [ ! -d "$dir" ]; then
    mkdir -p "$dir"
    cat > "$dir/boost_system-config.cmake" <<EOF
# Generated for conda-forge's split boost $ver packaging, which omits the
# header-only boost_system component that BoostConfig.cmake still requests.
if(TARGET Boost::system)
  return()
endif()
mark_as_advanced(boost_system_DIR)
get_filename_component(_BOOST_CMAKEDIR "\${CMAKE_CURRENT_LIST_DIR}/../" REALPATH)
get_filename_component(_BOOST_INCLUDEDIR "\${_BOOST_CMAKEDIR}/../../include/" ABSOLUTE)
add_library(Boost::system INTERFACE IMPORTED)
set_target_properties(Boost::system PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "\${_BOOST_INCLUDEDIR}"
)
unset(_BOOST_INCLUDEDIR)
unset(_BOOST_CMAKEDIR)
set(boost_system_FOUND TRUE)
EOF
    cat > "$dir/boost_system-config-version.cmake" <<EOF
set(PACKAGE_VERSION "$ver")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
EOF
    echo "[env repair] wrote missing boost_system component config for $ver"
  fi
}
repair_boost_system_component

export CC="clang-${CLANG_MAJOR}"
export CXX="clang++-${CLANG_MAJOR}"

# -idirafter /usr/include: conda clang is built with --sysroot, so system dev
# headers are invisible; appended, so sysroot glibc headers still win.
# -include stdcxx_compat.h: restores std headers that libstdc++ 9 leaked
# transitively and newer libstdc++ no longer includes.
# -Wno-format-truncation: clang 23 diagnoses snprintf() calls that provably
# truncate (libs/wireshark/epan/dissectors has several, e.g.
# packet-5co-legacy.c:923), and vendored wireshark compiles with -Werror.
# Disabled globally rather than patching ~1700 vendored dissectors.
COMMON_FLAGS="-march=nocona -mtune=haswell -ftree-vectorize -fPIC \
-fstack-protector-strong -fno-plt -O2 -ffunction-sections -pipe \
-Wno-format-truncation \
-isystem $CONDA_PREFIX/include -idirafter /usr/include \
--gcc-install-dir=$CONDA_PREFIX/lib/gcc/x86_64-conda-linux-gnu/16.2.0"
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
-include $HOME/.issue46-shim/stdcxx_compat.h"
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
