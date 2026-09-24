#!/usr/bin/env bash
set -eux

if [ "${CI:-}" != "true" ]; then
    echo "Please use the GitHub Action."
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR/..

OLD_VERSION="$1"
NEW_VERSION="$2"

IFS=. read -r NEW_MAJOR NEW_MINOR NEW_PATCH <<< "${NEW_VERSION%%[-+]*}"
echo "Bumping version: ${NEW_VERSION} (${NEW_MAJOR:-0}.${NEW_MINOR:-0}.${NEW_PATCH:-0})"

perl -pi -e "s/^(#\s*)define SENTRY_SDK_VERSION\s+.*/\$1define SENTRY_SDK_VERSION \"${NEW_VERSION}\"/" include/sentry.h
perl -pi -e "s/^(#\s*)define SENTRY_SDK_VERSION_MAJOR\s+.*/\$1define SENTRY_SDK_VERSION_MAJOR ${NEW_MAJOR:-0}/" include/sentry.h
perl -pi -e "s/^(#\s*)define SENTRY_SDK_VERSION_MINOR\s+.*/\$1define SENTRY_SDK_VERSION_MINOR ${NEW_MINOR:-0}/" include/sentry.h
perl -pi -e "s/^(#\s*)define SENTRY_SDK_VERSION_PATCH\s+.*/\$1define SENTRY_SDK_VERSION_PATCH ${NEW_PATCH:-0}/" include/sentry.h
perl -pi -e "s/^versionName\=.*/versionName\=${NEW_VERSION}/" ndk/gradle.properties
perl -pi -e "s/^SENTRY_VERSION \=.*/SENTRY_VERSION \= \"${NEW_VERSION}\"/" tests/__init__.py
