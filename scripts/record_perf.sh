#!/usr/bin/env bash

sudo perf record -e cycles -e context-switches -c 1 --sample-cpu -m 8M --aio -z --call-graph dwarf -- bin/bt_fuzzer --no-gui
sudo chown $USER perf.data
