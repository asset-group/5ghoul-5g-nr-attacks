#!/usr/bin/env bash
set -euo pipefail
P=${1:-${CONDA_PREFIX:?Usage: install_mongosh.sh CONDA_PREFIX}}
P=$(realpath -- "$P")
export PATH="$P/bin:$PATH"
[[ -x "$P/bin/node" && -x "$P/bin/npm" ]] || {
    echo 'The conda environment must provide nodejs and npm' >&2
    exit 1
}
"$P/bin/npm" install --global --prefix "$P" --no-audit --no-fund mongosh@2.14.3
"$P/bin/mongosh" --version
