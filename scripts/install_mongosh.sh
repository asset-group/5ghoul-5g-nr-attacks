#!/usr/bin/env bash
set -euo pipefail
P=${1:-${CONDA_PREFIX:?Usage: install_mongosh.sh CONDA_PREFIX}}
P=$(realpath -- "$P")
export PATH="$P/bin:$PATH"
[[ -x "$P/bin/node" && -x "$P/bin/npm" ]] || {
    echo 'The conda environment must provide nodejs and npm' >&2
    exit 1
}
if [[ -x "$P/bin/mongosh" ]] && [[ $("$P/bin/mongosh" --version) == 2.14.3 ]]; then
    echo 'mongosh 2.14.3 already installed'
    exit 0
fi
"$P/bin/npm" install --global --prefix "$P" --no-audit --no-fund mongosh@2.14.3
"$P/bin/mongosh" --version
