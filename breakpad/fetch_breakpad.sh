#!/usr/bin/env bash
###
# Fetch breakpad
###
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR

mkdir -p build/
cd build

if [ -d breakpad ]; then
  pushd breakpad/
  git pull -r
  popd
else
  git clone https://chromium.googlesource.com/breakpad/breakpad.git --depth=1
fi

if [ -d third_party/lss ]; then
  pushd third_party/lss
  git pull -r
  popd
else
  mkdir -p third_party
  pushd third_party
  git clone https://chromium.googlesource.com/linux-syscall-support.git lss --depth=1
  popd
fi

ln -s ../../../third_party/lss breakpad/src/third_party/lss
