#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "$0")/.." && pwd)
L=$(realpath -- "${1:?Usage: build_open5gs.sh CAMPAIGN [CONDA_PREFIX]}")
C="$ROOT/3rd-party/open5gs-core"
P=${2:-$HOME/miniforge3/envs/5ghoul23}
export PATH="$P/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export PKG_CONFIG_PATH="$L/mongo-driver-dist/lib/pkgconfig:$P/lib/pkgconfig:$P/share/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig"
export PKG_CONFIG=/usr/bin/pkg-config
unset LD_LIBRARY_PATH LD_PRELOAD CPPFLAGS CMAKE_ARGS CMAKE_OPTIONS
export CC="$P/bin/clang-23" CXX="$P/bin/clang++-23"
export CFLAGS="--gcc-install-dir=$P/lib/gcc/x86_64-conda-linux-gnu/16.2.0 -idirafter /usr/include"
export CXXFLAGS="$CFLAGS"
# Preserve build.sh's exact relative rpath, adding the new local dependencies.
export LDFLAGS="-L/usr/lib/x86_64-linux-gnu -Wl,-rpath=\$ORIGIN:\$ORIGIN/../libs/x86_64-linux-gnu -Wl,-rpath,$L/mongo-driver-dist/lib:$P/lib"
cd "$C"
# Seed the exact wrap revisions explicitly; Meson itself stays nodownload.
for PROJECT in freeDiameter libtins; do
  WRAP="$C/subprojects/$PROJECT.wrap"
  URL=$(sed -n 's/^url *= *//p' "$WRAP")
  REVISION=$(sed -n 's/^revision *= *//p' "$WRAP")
  DIR="$C/subprojects/$PROJECT"
  if [[ ! -e "$DIR/.git" ]]; then
    git clone --depth 1 --branch "$REVISION" "$URL" "$DIR" >> "$L/open5gs-subprojects-prepare.log" 2>&1
  fi
  [[ $(git -C "$DIR" rev-parse HEAD) == "$(git -C "$DIR" rev-parse "$REVISION^{commit}")" ]] || {
    echo "$PROJECT checkout does not match wrap revision $REVISION" >&2; exit 1;
  }
  printf '%s %s ' "$PROJECT" "$REVISION" >> "$L/open5gs-subprojects-prepare.log"
  git -C "$DIR" rev-parse HEAD >> "$L/open5gs-subprojects-prepare.log"
done
PATCH="$ROOT/patches/freeDiameter-r1.5.0.patch"
if ! git -C "$C/subprojects/freeDiameter" apply -R --check "$PATCH" 2>/dev/null; then
  git -C "$C/subprojects/freeDiameter" apply --check "$PATCH"
  git -C "$C/subprojects/freeDiameter" apply "$PATCH"
fi
ARGS=(meson setup build --wrap-mode=nodownload "--prefix=$C/install")
if [[ -f build/meson-private/coredata.dat ]]; then
  ARGS+=(--reconfigure --clearcache)
fi
{
  printf 'CC=%q CXX=%q CFLAGS=%q CXXFLAGS=%q LDFLAGS=%q PKG_CONFIG=%q PKG_CONFIG_PATH=%q ' "$CC" "$CXX" "$CFLAGS" "$CXXFLAGS" "$LDFLAGS" "$PKG_CONFIG" "$PKG_CONFIG_PATH"
  printf '%q ' "${ARGS[@]}"
  printf '\n'
} > "$L/open5gs-meson-fixed.command"
set +e
/usr/bin/time -f 'elapsed_seconds=%e' -o "$L/open5gs-meson-fixed.time" \
  "${ARGS[@]}" > "$L/open5gs-meson-fixed.log" 2>&1
RC=$?
printf '%s\n' "$RC" > "$L/open5gs-meson-fixed.exit"
set -e
if ((RC)); then cat "$L/open5gs-meson-fixed.log"; exit "$RC"; fi
set +e
/usr/bin/time -f 'elapsed_seconds=%e' -o "$L/open5gs-ninja-fixed.time" \
  ninja -C build -j8 > "$L/open5gs-ninja-fixed.log" 2>&1
RC=$?
printf '%s\n' "$RC" > "$L/open5gs-ninja-fixed.exit"
set -e
if ((RC)); then cat "$L/open5gs-ninja-fixed.log"; exit "$RC"; fi
printf 'Open5GS meson/ninja: 0/0; binary=%s/build/tests/app/5gc\n' "$C"
