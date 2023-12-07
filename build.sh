#!/usr/bin/env bash

# Configure deploy token
if [[ -v CI_DEPLOY_USER ]]
then
    echo "Configuring credentials for CI/CD..."
    git config --global credential.helper store
    echo "https://$CI_DEPLOY_USER:$CI_DEPLOY_PASSWORD@gitlab.com" > ~/.git-credentials
fi

# Source qt 5.12.2 environment for ubuntu 18.04, not needed for ubuntu 20.04 and beyond
if [ -f "/opt/qt512/bin/qt512-env.sh" ]
then
    source /opt/qt512/bin/qt512-env.sh
fi

# Make sure that .config is used
git config --local include.path ../.gitconfig || true

set -eo pipefail

if [ "$1" == "all" ]
then
    if [ "$(uname -m)" == "aarch64" ]
    then
        echo "Building GUI is not support on arm."
        exit 1
    fi
    mkdir -p build
    cmake -B build
    ninja -C build

elif [ "$1" == "dev" ]
then
    cmake -B build -DWIRESHARK_GUI=FALSE
    ninja -C build

elif [ "$1" == "fresh" ]
then
    sudo rm build -rdf
    cmake -B build
    ninja -C build

elif [ "$1" == "clean" ]
then
    ninja -C build clean

elif [ "$1" == "release" ]
then

    WS_HEADERS=$(find ./libs/wireshark -name "*.h")
    mkdir -p release
    cp version.json release/version.json
    cp container.sh release/container.sh
    echo "Creating compressed release/wdissector.tar.zst ..."
    tar -I 'zstd -c -T0 --ultra -20 -' --owner=1000 --group=1000 --exclude '*__pycache__*' --exclude '*.a' -cf release/wdissector.tar.zst --transform 'flags=r;s|^|wdissector/|' \
    README.md icon.png imgui.ini requirements.sh container.sh bin/ modules/ configs/ bindings/ scripts/python_env.sh scripts/test_* scripts/open5gs-dbctl.sh \
    scripts/packet_hex_to_c_array.py scripts/apply_patches.sh src/wdissector.h src/ModulesInclude.hpp src/ModulesStub.cpp libs/json.hpp src/GlobalConfig.hpp $WS_HEADERS \
    3rd-party/adb/ 3rd-party/uhubctl/ 3rd-party/usbip/ src/drivers/shm_interface examples/

    echo "Done!"

elif [ "$1" == "cicd" ]
then
    ./scripts/gitlab_test_ci.sh

elif [ "$1" == "doc" ]
then
    cd docs/old_greyhound
    mkdir -p ../.vuepress/public/old_greyhound
    node shins.js --inline --output ../.vuepress/public/old_greyhound/index.html
    cp -r source ../.vuepress/public/old_greyhound/
    cp -r app ../.vuepress/public/old_greyhound/
    cd ../../
    npm run build
    npm run pdf

elif [ "$1" == "doc_serve" ]
then
    cd docs/old_greyhound
    mkdir -p ../.vuepress/public/old_greyhound
    node shins.js --inline --output ../.vuepress/public/old_greyhound/index.html
    cp -r source ../.vuepress/public/old_greyhound/
    cp -r app ../.vuepress/public/old_greyhound/
    cd ../../
    npm run build_only

elif [ "$1" == "patches" ]
then
    ./scripts/gen_patches.sh 3rd-party
    ./scripts/gen_patches.sh libs

elif [ "$1" == "clean_gitlab_artifacts" ]
then
	npm install node-fetch
    shift
	node scripts/clean_gitlab_artifacts.js "$@"
else
    if [ ! -f "build/CMakeCache.txt" ]
    then
        if [ "$(uname -m)" == "aarch64" ]
        then
            cmake -B build -DWIRESHARK_GUI=FALSE
        else
            cmake -B build
        fi
    fi
    ninja -C build
fi
