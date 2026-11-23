#!/usr/bin/env bash
# Apply the 5Ghoul SHM patch to a VANILLA OpenAirInterface v3.0.0 checkout.
#
#   scripts/apply_oai_shm_patch.sh <path-to-oai-checkout>
#
# The patch is 5Ghoul-specific and targets exactly tag v3.0.0 (6f89078).
# It is NOT applied through scripts/apply_patches.sh, because that script
# derives the target submodule from the patch filename and only handles the
# vendored 3rd-party/ submodules (oai_5g_sa is pinned at 2023.w03).
set -euo pipefail

PATCH_NAME="oai-shm-v3.0.0.patch"
EXPECTED_TAG="v3.0.0"
EXPECTED_HEAD="6f89078"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
PATCH="$ROOT/patches/$PATCH_NAME"
SHM_DIR="$ROOT/src/drivers/shm_interface"

die() { echo "ERROR: $*" >&2; exit 1; }

[ $# -eq 1 ] || die "usage: $0 <path-to-oai-v3.0.0-checkout>"
OAI="$1"
[ -d "$OAI" ] || die "no such directory: $OAI"
[ -f "$PATCH" ] || die "patch not found: $PATCH"
[ -d "$SHM_DIR" ] || die "shm driver dir not found: $SHM_DIR"
[ -d "$OAI/.git" ] || die "$OAI is not a git checkout (needed for git apply + version check)"

cd "$OAI"
TAG="$(git describe --tags 2>/dev/null || true)"
HEAD="$(git rev-parse --short HEAD)"
echo "target : $OAI"
echo "version: ${TAG:-unknown} (HEAD $HEAD)"
case "$TAG" in
  "$EXPECTED_TAG"*) : ;;
  *) die "expected $EXPECTED_TAG, found '${TAG:-unknown}' — refusing to patch" ;;
esac

# The OAI sources include "shm_interface/wd_shm_nr_utils.h" and compile
# ${OPENAIR_DIR}/shm_interface/wd_shm.c, i.e. a symlink into 5Ghoul's driver.
echo "symlink: $OAI/shm_interface -> $SHM_DIR"
ln -sfn "$SHM_DIR" "$OAI/shm_interface"

echo "checking patch applies cleanly..."
if ! git apply --check "$PATCH"; then
  die "patch does not apply cleanly (wrong version or already patched)"
fi

echo "applying $PATCH_NAME"
git apply --whitespace=nowarn "$PATCH"
echo "OK: SHM patch applied to $OAI"
echo
echo "next: configure and build, e.g."
echo "  mkdir -p $OAI/build && cd $OAI/build"
echo "  cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo \\"
echo "        -DAUTO_DOWNLOAD_ASN1C=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
echo "  ninja nr-softmodem nr-uesoftmodem"
