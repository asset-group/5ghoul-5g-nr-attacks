#!/bin/bash

cd ../libs/wireshark
grep -iRl "CMAKE_SOURCE_DIR" ./ --include \*.cmake --include \*.txt | xargs -i@ sed -i 's/CMAKE_SOURCE_DIR/PROJECT_SOURCE_DIR/g' @
# grep -iRl "CMAKE_BINARY_DIR" ./ --include \*.cmake --include \*.txt | xargs -i@ sed -i 's/CMAKE_BINARY_DIR/PROJECT_BINARY_DIR/g' @