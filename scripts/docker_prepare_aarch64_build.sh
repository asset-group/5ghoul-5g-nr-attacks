#!/usr/bin/env bash


docker run --rm --privileged multiarch/qemu-user-static --reset -p yes --credential yes
# https://github.com/multiarch/qemu-user-static/issues/17
