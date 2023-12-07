#!/usr/bin/env bash

set -eo pipefail

# Go to script current path
ROOT="$(realpath $(dirname ${BASH_SOURCE[0]:-$0})/../)"

cd $ROOT/$1/

for p in *.patch
do
  PATCH_FOLDER=$(basename $p .patch)
  echo "-----------$1/$PATCH_FOLDER-----------"
  echo "Generating $p"
  if [ -d $ROOT/$1/$PATCH_FOLDER/.git ];
  then
    # if .git is a folder
    cd $ROOT/$1/$PATCH_FOLDER
  else
    # if .git is a file that points to the root project
    cd $ROOT/.git/modules/$1/$PATCH_FOLDER || true
  fi

  # Gen patch
  git diff --cached > /tmp/$p

  if [ -s /tmp/$p ]
  then
    mv -f /tmp/$p $ROOT/$1/$p
  else
    echo "Patch $p is empty. Don't forget to stage files"
  fi
  echo "OK"
done
