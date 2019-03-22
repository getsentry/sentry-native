#!/usr/bin/env bash
set -eu

# Requirements: curl, unzip

BUILD_REPO="https://github.com/getsentry/build-pad"
TMP_FILE=".file.download.tmp.zip"
CRASHTOOL=${1:-}
PLATFORM="$(uname -s)"
ARCH="$(uname -m)"
DEPS_DIR="deps"
OUT_DIR="${DEPS_DIR}/${CRASHTOOL}-${PLATFORM}"
FILE_URL="${BUILD_REPO}/releases/latest/download/${CRASHTOOL}-${PLATFORM}-${ARCH}.zip"

if [[ "$CRASHTOOL" == "crashpad" ]]; then
    FILES_TO_CHECK=(
        "${OUT_DIR}/include/client/crashpad_client.h"
    )
elif [[ "$CRASHTOOL" == "breakpad" ]]; then
    FILES_TO_CHECK=(
        "${OUT_DIR}/include/client/minidump_file_writer.h"
    )
else
    echo "Usage: $0 crashpad|breakpad"
    exit 1
fi

cleanup() {
    rm -f "${TMP_FILE}"
}
trap cleanup EXIT

download_and_extract() {
    echo "Downloading ${CRASHTOOL}"
    rm -f "${TMP_FILE}"
    mkdir -p deps
    curl --silent --show-error --fail -L "${FILE_URL}" -o "${TMP_FILE}"
    unzip -o "${TMP_FILE}" -d "${DEPS_DIR}"
}

DOWNLOAD_IF_NOT_EXIST="${IF_NOT_EXIST:-}"

FILES_EXIST=""
if ls ${FILES_TO_CHECK[@]} 2>&1 >/dev/null; then
    FILES_EXIST="1"
fi

if [[ "$DOWNLOAD_IF_NOT_EXIST" == "1" && "$FILES_EXIST" == "1" ]]; then
    echo 'Files already exist, not downloading.'
    exit 0
else
    rm -rf "${OUT_DIR}"
    download_and_extract
fi
