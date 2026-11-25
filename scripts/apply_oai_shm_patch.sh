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
EXPECTED_HEAD="6f89078b409e671fa566782f67aae9de52fd55d8"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
PATCH="$ROOT/patches/$PATCH_NAME"
SHM_DIR="$ROOT/src/drivers/shm_interface"

die() { echo "ERROR: $*" >&2; exit 1; }

[ $# -eq 1 ] || die "usage: $0 <path-to-oai-v3.0.0-checkout>"
[ -d "$1" ] || die "no such directory: $1"
OAI="$(cd "$1" && pwd -P)"
[ -f "$PATCH" ] || die "patch not found: $PATCH"
[ -d "$SHM_DIR" ] || die "shm driver dir not found: $SHM_DIR"
[ -e "$OAI/.git" ] || die "$OAI is not a git checkout"
[ "$(git -C "$OAI" rev-parse --show-toplevel)" = "$OAI" ] || die "expected the checkout root"

cd "$OAI"
HEAD="$(git rev-parse HEAD)"
TAG_HEAD="$(git rev-parse --verify "refs/tags/$EXPECTED_TAG^{commit}" 2>/dev/null || true)"
echo "target : $OAI"
echo "version: HEAD $HEAD"
[ "$HEAD" = "$EXPECTED_HEAD" ] && [ "$TAG_HEAD" = "$EXPECTED_HEAD" ] ||
  die "expected exactly $EXPECTED_TAG ($EXPECTED_HEAD); refusing to patch"
git diff --cached --quiet || die "staged changes present; refusing to patch"

# Construct the expected patched index privately. Comparing the worktree to
# it rejects extra tracked edits even when git apply --reverse --check passes.
# The real index is never changed; .git may be a worktree metadata file.
TMP="$(mktemp -d)"
trap 'rm -rf -- "$TMP"' EXIT
INDEX="$TMP/index"
GIT_INDEX_FILE="$INDEX" git read-tree HEAD
GIT_INDEX_FILE="$INDEX" git apply --cached --whitespace=nowarn "$PATCH"

if GIT_INDEX_FILE="$INDEX" git diff --quiet; then
  STATE=patched
elif git diff --quiet && git apply --check "$PATCH"; then
  STATE=vanilla
else
  die "checkout is neither vanilla nor exactly SHM-patched; preserving sources and symlink"
fi

# Only a symlink may be replaced, and only after all source checks succeeded.
if [ -e "$OAI/shm_interface" ] && [ ! -L "$OAI/shm_interface" ]; then
  die "shm_interface is not a symlink; refusing to replace it"
fi
if [ "$STATE" = vanilla ]; then
  git apply --whitespace=nowarn "$PATCH"
fi
if [ ! -L "$OAI/shm_interface" ] || [ "$(readlink "$OAI/shm_interface")" != "$SHM_DIR" ]; then
  ln -sfnT "$SHM_DIR" "$OAI/shm_interface"
fi
echo "OK: exact SHM patch present in $OAI"
