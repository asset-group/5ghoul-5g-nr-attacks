#!/usr/bin/env bash
# Build the Open5GS C driver with the single default conda toolchain.
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "$0")/.." && pwd)
CAMPAIGN=$(realpath -- "${1:?Usage: build_mongo_driver.sh CAMPAIGN [CONDA_PREFIX]}")
PREFIX=${2:-$HOME/miniforge3/envs/5ghoul23}
SOURCE="$CAMPAIGN/mongo-c-driver"
BUILD="$SOURCE/build-fixed"
DIST="$CAMPAIGN/mongo-driver-dist"
PATCH="$ROOT/patches/mongo-c-driver-1.30.2.patch"
GCC_DIR="$PREFIX/lib/gcc/x86_64-conda-linux-gnu/16.2.0"
export PATH="$PREFIX/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
unset CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS LD_LIBRARY_PATH LD_PRELOAD
unset CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_ARGS CMAKE_OPTIONS
[[ -d "$GCC_DIR" ]] || { echo "Missing GCC16.2.0 headers: $GCC_DIR" >&2; exit 1; }
[[ -x "$PREFIX/bin/clang-23" && -x "$PREFIX/bin/clang++-23" ]] || {
  echo "Missing clang23 toolchain in $PREFIX" >&2; exit 1;
}
if [[ ! -d "$SOURCE/.git" ]]; then
  git clone --depth 1 --branch 1.30.2 https://github.com/mongodb/mongo-c-driver.git "$SOURCE"
fi
[[ $(git -C "$SOURCE" rev-parse HEAD) == "$(git -C "$SOURCE" rev-parse '1.30.2^{commit}')" ]] || {
  echo 'Driver checkout is not the pinned 1.30.2 commit' >&2; exit 1;
}
if git -C "$SOURCE" apply -R --check "$PATCH" 2>/dev/null; then
  echo 'MongoDB C-driver compatibility patch already applied'
else
  git -C "$SOURCE" apply --check "$PATCH"
  git -C "$SOURCE" apply "$PATCH"
fi
FLAGS="--gcc-install-dir=$GCC_DIR -idirafter /usr/include"
ARGS=(cmake -S "$SOURCE" -B "$BUILD" -GNinja
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
  "-DCMAKE_INSTALL_PREFIX=$DIST"
  "-DCMAKE_C_COMPILER=$PREFIX/bin/clang-23"
  "-DCMAKE_CXX_COMPILER=$PREFIX/bin/clang++-23"
  "-DCMAKE_C_FLAGS=$FLAGS" "-DCMAKE_CXX_FLAGS=$FLAGS"
  "-DCMAKE_PREFIX_PATH=$PREFIX"
  "-DCMAKE_INSTALL_RPATH=$DIST/lib;$PREFIX/lib"
  -DENABLE_STATIC=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF)
printf '%q ' "${ARGS[@]}" > "$CAMPAIGN/mongo-driver-configure-fixed.command"
printf '\n' >> "$CAMPAIGN/mongo-driver-configure-fixed.command"
set +e
/usr/bin/time -f 'elapsed_seconds=%e' -o "$CAMPAIGN/mongo-driver-configure-fixed.time" \
  "${ARGS[@]}" > "$CAMPAIGN/mongo-driver-configure-fixed.log" 2>&1
RC=$?
printf '%s\n' "$RC" > "$CAMPAIGN/mongo-driver-configure-fixed.exit"
set -e
if ((RC)); then cat "$CAMPAIGN/mongo-driver-configure-fixed.log"; exit "$RC"; fi
set +e
/usr/bin/time -f 'elapsed_seconds=%e' -o "$CAMPAIGN/mongo-driver-ninja-fixed.time" \
  ninja -C "$BUILD" -j8 > "$CAMPAIGN/mongo-driver-ninja-fixed.log" 2>&1
RC=$?
printf '%s\n' "$RC" > "$CAMPAIGN/mongo-driver-ninja-fixed.exit"
set -e
if ((RC)); then cat "$CAMPAIGN/mongo-driver-ninja-fixed.log"; exit "$RC"; fi
set +e
/usr/bin/time -f 'elapsed_seconds=%e' -o "$CAMPAIGN/mongo-driver-install-fixed.time" \
  cmake --install "$BUILD" > "$CAMPAIGN/mongo-driver-install-fixed.log" 2>&1
RC=$?
printf '%s\n' "$RC" > "$CAMPAIGN/mongo-driver-install-fixed.exit"
set -e
if ((RC)); then cat "$CAMPAIGN/mongo-driver-install-fixed.log"; exit "$RC"; fi
printf 'Driver configure/build/install: 0/0/0; dist=%s\n' "$DIST"
