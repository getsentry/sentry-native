#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

mkdir -p build
cd build

# Install depot_tools
if [ -d depot_tools ]; then
  pushd depot_tools
  git pull -r
  popd
else
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi

. $SCRIPT_DIR/vars.sh

if [[ "${TF_BUILD:-}" == "True" ]]; then
  # Set git data to avoid errors
  git config --global user.email "ci.build.crashpad@sentry.io"
  git config --global user.name "CI build"
fi

$FETCH_CMD crashpad
cd crashpad
git remote add getsentry https://github.com/getsentry/crashpad
git fetch getsentry getsentry
git checkout -f getsentry/getsentry
git log -1
