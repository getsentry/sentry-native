#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BASE_DIR="$SCRIPT_DIR/.."
OUT_DIR="$BASE_DIR/out"
TARGET_REVISION="${1:-HEAD}"

rm -rf "$OUT_DIR"
mkdir $OUT_DIR

fetch_crashpad() {
    echo '
#########################
### Fetching Crashpad ###
#########################
    '
    ### Crashpad
    # CRASHPAD_REMOTE="git@github.com:getsentry/crashpad.git"
    # CRASHPAD_REVISION="origin/getsentry"
    CRASHPAD_OUT_DIR="$OUT_DIR/crashpad"
    mkdir -p "$CRASHPAD_OUT_DIR"

    # FIXME this should be a clean sentry-native checkout
    CRASHPAD_IN_DIR="$BASE_DIR/crashpad"
    CRASHPAD_COPY_SRC=("examples" "fetch_crashpad.sh" "vars.sh")

    # Copy files
    for f in "${CRASHPAD_COPY_SRC[@]}"; do
        cp -r "$CRASHPAD_IN_DIR/$f" "$CRASHPAD_OUT_DIR/"
    done

    # Fetch crashpad and its dependencies
    bash "$CRASHPAD_OUT_DIR/fetch_crashpad.sh"
    # Clean up unneeded files
    rm -rf $CRASHPAD_OUT_DIR/build/{depot_tools,buildtools} \
           $CRASHPAD_OUT_DIR/build/crashpad/third_party/{gtest,gyp} \
           $CRASHPAD_OUT_DIR/build/crashpad/{.git,doc,test}
}


fetch_breakpad() {
    echo '
#########################
### Fetching Breakpad ###
#########################
    '
    BREAKPAD_OUT_DIR="$OUT_DIR/breakpad"
    mkdir -p "$BREAKPAD_OUT_DIR"

    BREAKPAD_IN_DIR="$BASE_DIR/breakpad"
    BREAKPAD_COPY_SRC=("examples" "fetch_breakpad.sh")

    # Copy files
    for f in "${BREAKPAD_COPY_SRC[@]}"; do
        cp -r "$BREAKPAD_IN_DIR/$f" "$BREAKPAD_OUT_DIR/"
    done

    # Fetch breakpad and its dependencies
    bash "$BREAKPAD_OUT_DIR/fetch_breakpad.sh"
    rm -rf $BREAKPAD_OUT_DIR/build/breakpad/{.git,docs} \
           $BREAKPAD_OUT_DIR/build/breakpad/src/{tools,processor}
}


fetch_sentry_native() {
    echo '
##############################
### Fetching Sentry Native ###
##############################
    '
    SENTRY_NATIVE_REMOTE="https://github.com/getsentry/sentry-native/"
    SENTRY_NATIVE_REVISION=$(git rev-parse "$TARGET_REVISION")
    SENTRY_NATIVE_IN_DIR="$BASE_DIR/.sentry-native-tmp"
    if [ -d "$SENTRY_NATIVE_IN_DIR" ]; then
        cd "$SENTRY_NATIVE_IN_DIR"
        git fetch origin
        git checkout -f "$SENTRY_NATIVE_REVISION"
    else
        git clone "$SENTRY_NATIVE_REMOTE" "$SENTRY_NATIVE_IN_DIR"
        cd "$SENTRY_NATIVE_IN_DIR"
        git checkout -f "$SENTRY_NATIVE_REVISION"
    fi

    SENTRY_NATIVE_SRC=(
        "examples" "include" "src" "premake" "README.md" "Makefile"
        "tests" "Dockerfile" ".dockerignore" "android"
    )

    # Copy files
    for f in "${SENTRY_NATIVE_SRC[@]}"; do
        cp -r "$SENTRY_NATIVE_IN_DIR/$f" "$OUT_DIR/"
    done
}

fetch_sentry_native
fetch_crashpad
fetch_breakpad
