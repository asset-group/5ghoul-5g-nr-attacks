#!/usr/bin/env bash

# Runtime subprocess tools use the same default environment as build.sh.
export PATH="${MINIFORGE_ROOT:-$HOME/miniforge3}/envs/5ghoul23/bin:$PATH"

alias pip3="python3 -m pip"
