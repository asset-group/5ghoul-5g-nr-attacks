#!/usr/bin/env bash


echo -e "\n\nconst char *$1 = R\"/($(cat $2))/\";" >> $3
