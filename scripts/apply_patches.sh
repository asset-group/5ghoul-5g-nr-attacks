#!/usr/bin/env bash

# Go to script current path
cd $(dirname ${BASH_SOURCE[0]:-$0})
# Go to project root path
cd ../

# Initialize all submodules in folder $1
git submodule update --init --depth 1 $1

# Ensure submodules are in the correct commit before applying patch
for p in $1/*.patch
do
    PATCH_FOLDER=$(basename "$p" .patch)
    git submodule update --init --depth 1 $1/$PATCH_FOLDER || true

    if [[ $? -eq 1 ]]
    then
        echo "Commits have changed, checking out clean submodules in $1"
        git submodule update --init --force --depth 1 $1/$PATCH_FOLDER || true
    fi
done


# Apply patches
cd $1

for p in *.patch
do
 echo "-----------$1/$p-----------"
 echo "Verifying patch $p"
 PATCH_FOLDER=$(basename "$p" .patch)
 cd $PATCH_FOLDER
 echo "$(pwd)"
 git apply -R --check "../$p" &> /dev/null
 PATCH_APPLIED=$?
 if [[ $? -eq 0 ]] && [[ -z "$(git diff --cached)" ]] && [[ $PATCH_APPLIED -eq 1 ]]
 then
    echo "Cleaning up repo for new patch"
    sudo git reset --hard || true
    sudo git clean -ffdx || true
    cd ../
 else
    cd ../
    echo "Skipping patch"
    continue
 fi

 echo "Applying patch $p"

 cp $p $PATCH_FOLDER -f
 cd $PATCH_FOLDER
 sudo git apply $p  > /dev/null || true
 rm $p
 cd ../
 echo "OK"
done
