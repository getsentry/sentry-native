#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUT_DIR="$SCRIPT_DIR/out"
rm -rf "$OUT_DIR"
mkdir $OUT_DIR
cp README.package.md "$OUT_DIR/README.md"

CONFIGS=("macosx_gmake2" "linux_gmake2" "windows_vs2017")

### Sentrypad
SENTRYPAD_REMOTE="https://github.com/getsentry/sentrypad/"
SENTRYPAD_REVISION=$(git rev-parse ${1:-HEAD})
SENTRYPAD_IN_DIR="$SCRIPT_DIR/.sentrypad-tmp"
if [ -d "$SENTRYPAD_IN_DIR" ]; then
    cd "$SENTRYPAD_IN_DIR"
    git fetch origin
    git checkout -f "$SENTRYPAD_REVISION"
else
    git clone "$SENTRYPAD_REMOTE" "$SENTRYPAD_IN_DIR"
    cd "$SENTRYPAD_IN_DIR"
    git checkout -f "$SENTRYPAD_REVISION"
fi

SENTRYPAD_OUT_DIR="$OUT_DIR"
SENTRYPAD_SRC=("example.c" "include" "src" "premake")

mkdir -p $SENTRYPAD_OUT_DIR

# Copy files
for f in "${SENTRYPAD_SRC[@]}"; do
    cp -r "$SENTRYPAD_IN_DIR/$f" "$SENTRYPAD_OUT_DIR/"
done


### Crashpad
# CRASHPAD_REMOTE="git@github.com:getsentry/crashpad.git"
# CRASHPAD_REVISION="origin/getsentry"
CRASHPAD_OUT_DIR="$OUT_DIR/crashpad"

# FIXME this should be a clean sentrypad checkout
CRASHPAD_IN_DIR="$SCRIPT_DIR/crashpad"
CRASHPAD_COPY_SRC=("examples" "fetch_crashpad.sh" "vars.sh")

# Copy files
mkdir -p "$CRASHPAD_OUT_DIR"
for f in "${CRASHPAD_COPY_SRC[@]}"; do
    cp -r "$CRASHPAD_IN_DIR/$f" "$CRASHPAD_OUT_DIR/"
done

# Fetch crashpad and its dependencies
bash "$CRASHPAD_OUT_DIR/fetch_crashpad.sh"
# Clean up unneeded files
rm -rf $CRASHPAD_OUT_DIR/build/{depot_tools,buildtools}

##############################################
### Generate build files for all platforms ###
##############################################
for CONFIG in "${CONFIGS[@]}"; do
    PLATFORM="${CONFIG%_*}"
    BUILD_SYSTEM="${CONFIG#*_}"
    PLATFORM_GEN_DIR="$SENTRYPAD_OUT_DIR/gen_$PLATFORM"
    cp -r "$SENTRYPAD_OUT_DIR/premake" "$PLATFORM_GEN_DIR"
    cd $PLATFORM_GEN_DIR
    # Run premake for the given platform and build system
    premake5 "${BUILD_SYSTEM}" --os="$PLATFORM"
done
