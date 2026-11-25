#!/usr/bin/env bash
# Configure every time, then incrementally build with the installed conda env.
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
MINIFORGE=${MINIFORGE_ROOT:-$HOME/miniforge3}
PREFIX="$MINIFORGE/envs/5ghoul23"
[[ $# == 0 ]] || { echo 'usage: ./build.sh (configure + incremental Ninja)' >&2; exit 1; }
[[ -f "$MINIFORGE/etc/profile.d/conda.sh" && -d "$PREFIX/conda-meta" ]] || {
  echo 'Run ./requirements.sh first to install 5ghoul23' >&2; exit 1;
}
set +u
source "$MINIFORGE/etc/profile.d/conda.sh"
conda activate "$PREFIX"
set -u
GCCDIR="$PREFIX/lib/gcc/x86_64-conda-linux-gnu/16.2.0"
[[ -d "$GCCDIR" ]] || { echo "Missing GCC 16.2.0 headers: $GCCDIR" >&2; exit 1; }
export CC="$PREFIX/bin/clang-23" CXX="$PREFIX/bin/clang++-23"
export LD_LIBRARY_PATH="$PREFIX/lib"
unset LD_PRELOAD CPPFLAGS CMAKE_ARGS CMAKE_OPTIONS
COMMON="-march=nocona -mtune=haswell -ftree-vectorize -fPIC -fstack-protector-strong -fno-plt -O2 -ffunction-sections -pipe -Wno-format-truncation -isystem $PREFIX/include -idirafter /usr/include --gcc-install-dir=$GCCDIR"
export CFLAGS="$COMMON -Wno-error=incompatible-function-pointer-types -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion"
export CXXFLAGS="$COMMON -fvisibility-inlines-hidden -DGLOG_USE_GLOG_EXPORT -I$ROOT/libs/pybind11/include -include cstdint"
export LDFLAGS="--gcc-install-dir=$GCCDIR -L$PREFIX/lib -Wl,-rpath-link,$PREFIX/x86_64-conda-linux-gnu/sysroot/usr/lib -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu"
cd "$ROOT"
mkdir -p logs/build
ARGS=("$PREFIX/bin/cmake" -S "$ROOT" -B "$ROOT/build" -G Ninja
  -DWDS_CLANG_MAJOR=23
  "-DCMAKE_C_COMPILER=$CC" "-DCMAKE_CXX_COMPILER=$CXX"
  "-DCMAKE_C_FLAGS=$CFLAGS" "-DCMAKE_CXX_FLAGS=$CXXFLAGS"
  "-DCMAKE_EXE_LINKER_FLAGS=$LDFLAGS"
  "-DCMAKE_SHARED_LINKER_FLAGS=$LDFLAGS"
  "-DCMAKE_MODULE_LINKER_FLAGS=$LDFLAGS"
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  -DWIRESHARK_GUI=OFF -DENABLE_RPATH_ORIGIN=TRUE
  -DENABLE_WERROR=OFF -DENABLE_LUA=OFF
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
  "-DCMAKE_PREFIX_PATH=$PREFIX" "-DEIGEN3_INCLUDE_DIR=$PREFIX/include/eigen3"
  "-DPython3_EXECUTABLE=$PREFIX/bin/python3")
printf '%q ' "${ARGS[@]}" > logs/build/configure.command
printf '\n' >> logs/build/configure.command
"${ARGS[@]}" 2>&1 | tee logs/build/configure.log
"$PREFIX/bin/ninja" -C "$ROOT/build" -j "${JOBS:-8}" 2>&1 | tee logs/build/ninja.log
