#!/usr/bin/env bash

# Use ordered_json for all json instances, this preserve save ordering
sed -i "/using nlohmann::json;/a \ \ \ \ using nlohmann::ordered_json;" $1
sed -i "s/json::/ordered_json::/g" $1
sed -i "s/json \&/ordered_json \&/g" $1 
