#!/usr/bin/env bash
sudo chrt --rr 99 cset shield ./../example_wdissector b | tee wdissector-benchmark-$(uname -r).txt
