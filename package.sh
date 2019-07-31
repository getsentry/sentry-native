#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUT_DIR="$SCRIPT_DIR/out"
rm -rf "$OUT_DIR"
mkdir $OUT_DIR

fetch_crashpad() {
    ### Crashpad
    # CRASHPAD_REMOTE="git@github.com:getsentry/crashpad.git"
    # CRASHPAD_REVISION="origin/getsentry"
    CRASHPAD_OUT_DIR="$OUT_DIR/crashpad"
    mkdir -p "$CRASHPAD_OUT_DIR"

    # FIXME this should be a clean sentry-native checkout
    CRASHPAD_IN_DIR="$SCRIPT_DIR/crashpad"
    CRASHPAD_COPY_SRC=("examples" "fetch_crashpad.sh" "vars.sh")

    # Copy files
    for f in "${CRASHPAD_COPY_SRC[@]}"; do
        cp -r "$CRASHPAD_IN_DIR/$f" "$CRASHPAD_OUT_DIR/"
    done

    # Fetch crashpad and its dependencies
    bash "$CRASHPAD_OUT_DIR/fetch_crashpad.sh"
    # Clean up unneeded files
    rm -rf $CRASHPAD_OUT_DIR/build/{depot_tools,buildtools} $CRASHPAD_OUT_DIR/build/crashpad/third_party/{gtest,gyp}
}


fetch_breakpad() {
    ### Breakpad
    BREAKPAD_OUT_DIR="$OUT_DIR/breakpad"
    mkdir -p "$BREAKPAD_OUT_DIR"

    BREAKPAD_IN_DIR="$SCRIPT_DIR/breakpad"
    BREAKPAD_COPY_SRC=("fetch_breakpad.sh")

    # Copy files
    for f in "${BREAKPAD_COPY_SRC[@]}"; do
        cp -r "$BREAKPAD_IN_DIR/$f" "$BREAKPAD_OUT_DIR/"
    done

    # Fetch breakpad and its dependencies
    bash "$BREAKPAD_OUT_DIR/fetch_breakpad.sh"
}


### Sentry-Native
SENTRY_NATIVE_REMOTE="https://github.com/getsentry/sentry-native/"
SENTRY_NATIVE_REVISION=$(git rev-parse ${1:-HEAD})
SENTRY_NATIVE_IN_DIR="$SCRIPT_DIR/.sentry-native-tmp"
if [ -d "$SENTRY_NATIVE_IN_DIR" ]; then
    cd "$SENTRY_NATIVE_IN_DIR"
    git fetch origin
    git checkout -f "$SENTRY_NATIVE_REVISION"
else
    git clone "$SENTRY_NATIVE_REMOTE" "$SENTRY_NATIVE_IN_DIR"
    cd "$SENTRY_NATIVE_IN_DIR"
    git checkout -f "$SENTRY_NATIVE_REVISION"
fi

SENTRY_NATIVE_SRC=("example.c" "include" "src" "premake" "README.md")

# Copy files
for f in "${SENTRY_NATIVE_SRC[@]}"; do
    cp -r "$SENTRY_NATIVE_IN_DIR/$f" "$OUT_DIR/"
done

fetch_crashpad

fetch_breakpad
