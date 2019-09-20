#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# NDK version
cat ${ANDROID_NDK_HOME}/source.properties

cd ${SCRIPT_DIR}/sample_app

# Compile!
${ANDROID_NDK_HOME}/ndk-build --debug
