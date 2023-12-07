#!/usr/bin/env bash

set -eo pipefail

ARCH=$(uname -m)

# Ensure release folder is empty
mkdir -p release
rm release/* -rdf || true

if [ "$ARCH" == "x86_64" ]
then
    ./build.sh all
    ./requirements.sh doc
    ./build.sh doc
    ./build.sh release
    ./build.sh release_exploiter
else
    # Skip documentation for arm build
    ./build.sh dev
    ./build.sh release
fi

if [ "$ARCH" == "x86_64" ]
then
    # Build ESP32 BT Classic firmware
    if [[ -v CI_DEPLOY_USER ]]
    then
        echo "Fixing platformio env..."
        ln -sf $(pwd)/modules/python/install/bin/python3 /$HOME/.platformio/penv/bin/python3
    fi

    modules/python/install/bin/python3 src/drivers/firmware_bluetooth/firmware.py build
    # Copy zipped firmware to release folder
    cp src/drivers/firmware_bluetooth/release/esp32driver.zip release
fi
