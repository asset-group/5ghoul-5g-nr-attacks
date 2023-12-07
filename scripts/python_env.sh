#!/usr/bin/env bash

# Get script current path
CURRENT_PATH="$(readlink -e $(dirname ${BASH_SOURCE[0]:-$0}))"
export PATH="$CURRENT_PATH/../modules/python/install/bin/:$PATH"

alias pip3="python3 -m pip"
