#!/usr/bin/env bash

set -euo pipefail

die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

[[ $# -eq 1 ]] || die "usage: $0 <3rd-party|./libs|./modules>"
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
GROUP="${1#./}"
GROUP="${GROUP%/}"
case "$GROUP" in
    3rd-party|libs|modules) ;;
    *) die "unsupported submodule group: $1" ;;
esac
[[ -d "$ROOT/$GROUP" && ! -L "$ROOT/$GROUP" ]] || die "invalid group directory: $ROOT/$GROUP"
[[ "$(git -C "$ROOT" rev-parse --show-toplevel)" == "$ROOT" ]] || die "not the project repository: $ROOT"
cd -- "$ROOT"

# Only gitlinks registered in .gitmodules are eligible targets. Validate the
# complete group and its patches before allowing any submodule checkout.
if REGISTRATIONS="$(git config --file .gitmodules --get-regexp '^submodule\..*\.path$')"; then
    :
else
    STATUS=$?
    [[ $STATUS -eq 1 ]] || die "cannot read submodule registrations from $ROOT/.gitmodules"
    REGISTRATIONS=''
fi
GITLINKS="$(git --literal-pathspecs ls-files --stage -- "$GROUP")"
SUBMODULES=()
declare -A EXPECTED=()
while IFS=$'\t' read -r METADATA TARGET; do
    [[ -n "$METADATA" ]] || continue
    read -r MODE COMMIT STAGE <<< "$METADATA"
    [[ "$MODE" == 160000 ]] || continue
    [[ "$STAGE" == 0 ]] || die "unmerged gitlink: $TARGET"
    REGISTERED=false
    while read -r KEY REGISTERED_PATH; do
        if [[ "$REGISTERED_PATH" == "$TARGET" ]]; then
            REGISTERED=true
            break
        fi
    done <<< "$REGISTRATIONS"
    [[ "$REGISTERED" == true ]] || die "gitlink not registered in .gitmodules: $TARGET"
    SUBMODULES+=("$TARGET")
    EXPECTED["$TARGET"]="$COMMIT"
done <<< "$GITLINKS"

shopt -s nullglob
PATCHES=("$ROOT/$GROUP/"*.patch)
for PATCH in "${PATCHES[@]}"; do
    TARGET="$GROUP/$(basename -- "$PATCH" .patch)"
    [[ -f "$PATCH" ]] || die "not a patch file: $PATCH"
    [[ -n "${EXPECTED[$TARGET]+registered}" ]] || die "patch $PATCH has no registered gitlink target: $TARGET"
done

for TARGET in "${SUBMODULES[@]}"; do
    [[ ! -L "$ROOT/$TARGET" ]] || die "refusing symlinked submodule target: $TARGET"
    if [[ -e "$ROOT/$TARGET/.git" ]]; then
        [[ "$(git -C "$TARGET" rev-parse --show-toplevel)" == "$ROOT/$TARGET" ]] || die "unexpected repository at $TARGET"
        ACTUAL="$(git -C "$TARGET" rev-parse HEAD)"
        TRACKED_CHANGES="$(git -C "$TARGET" status --porcelain --untracked-files=no --ignore-submodules=none)"
        if [[ "$ACTUAL" != "${EXPECTED[$TARGET]}" && -n "$TRACKED_CHANGES" ]]; then
            die "$TARGET is at $ACTUAL, expected ${EXPECTED[$TARGET]}, with tracked changes; refusing checkout"
        fi
        # A recursive update can move nested checkouts even when this target
        # already has the right HEAD. Inspect them before updating any target.
        if ! git -C "$TARGET" submodule foreach --recursive '
            set -e
            actual="$(git rev-parse HEAD)"
            tracked_changes="$(git status --porcelain --untracked-files=no --ignore-submodules=none)"
            if test "$actual" != "$sha1" && test -n "$tracked_changes"; then
                printf "ERROR: %s is at %s, expected %s, with tracked changes; refusing checkout\n" "$displaypath" "$actual" "$sha1" >&2
                exit 1
            fi
        '; then
            die "nested submodule preflight failed for $TARGET"
        fi
    fi
done

if [[ ${#SUBMODULES[@]} -gt 0 ]]; then
    # Explicit checkout mode ignores local merge/rebase update preferences;
    # without force, Git refuses changes that would overwrite local work.
    if ! git submodule update --init --recursive --checkout -- "${SUBMODULES[@]}"; then
        die "submodule initialization failed for $GROUP; local files were not discarded"
    fi
else
    printf 'No registered submodules in %s\n' "$GROUP"
fi

for TARGET in "${SUBMODULES[@]}"; do
    ACTUAL="$(git -C "$TARGET" rev-parse HEAD)"
    [[ "$ACTUAL" == "${EXPECTED[$TARGET]}" ]] || die "$TARGET is at $ACTUAL, expected ${EXPECTED[$TARGET]}; refusing to patch"
done

for PATCH in "${PATCHES[@]}"; do
    TARGET="$GROUP/$(basename -- "$PATCH" .patch)"
    if git -C "$TARGET" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
        printf 'Already applied: %s -> %s\n' "$PATCH" "$TARGET"
        continue
    fi
    printf 'Checking patch: %s -> %s\n' "$PATCH" "$TARGET"
    if ! git -C "$TARGET" apply --check "$PATCH"; then
        die "patch $PATCH conflicts with $TARGET; no patch changes applied to this target"
    fi
    if ! git -C "$TARGET" apply "$PATCH"; then
        die "failed to apply patch $PATCH to $TARGET"
    fi
    printf 'Applied: %s -> %s\n' "$PATCH" "$TARGET"
done

if [[ ${#PATCHES[@]} -eq 0 ]]; then
    printf 'No patches in %s\n' "$GROUP"
fi
