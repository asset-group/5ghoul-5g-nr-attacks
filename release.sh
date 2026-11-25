#!/usr/bin/env bash
# Maintenance packaging; installation/build remain requirements.sh/build.sh.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
./build.sh
mkdir -p release
mapfile -t WS_HEADERS < <(find ./libs/wireshark -name '*.h')
tar -I 'zstd -c -T0 --ultra -20 -' --owner=1000 --group=1000 \
  --exclude '*__pycache__*' --exclude '*.a' \
  -cf release/wdissector.tar.zst --transform 'flags=r;s|^|wdissector/|' \
  README.md icon.png imgui.ini environment.yml container.sh bin/ modules/ \
  configs/ bindings/ scripts/python_env.sh scripts/test_* scripts/open5gs-dbctl.sh \
  scripts/packet_hex_to_c_array.py src/wdissector.h src/ModulesInclude.hpp \
  src/ModulesStub.cpp libs/json.hpp src/GlobalConfig.hpp "${WS_HEADERS[@]}" \
  3rd-party/adb/ 3rd-party/uhubctl/ 3rd-party/usbip/ src/drivers/shm_interface examples/
echo 'Created release/wdissector.tar.zst (runtime bundle; source installs use a Git clone)'
