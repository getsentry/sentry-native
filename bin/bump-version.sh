#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR/..

if [[ "$#" -eq 1 ]]; then
  NEW_VERSION="$1"
elif [[ "$#" -eq 2 ]]; then
  OLD_VERSION="$1"
  NEW_VERSION="$2"
else
  echo "Invalid number of arguments"
  exit 1
fi

sed -i '' -e "s/^#define SENTRY_SDK_VERSION.*/#define SENTRY_SDK_VERSION \"${NEW_VERSION}\"/" include/sentry.h
