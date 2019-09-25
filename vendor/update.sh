#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR

pull_premake_androidmk() {
  REMOTE="https://github.com/tonyo/premake-androidmk"
  BRANCH="sentry"
  PROJECT_DIR="premake-androidmk"

  rm -rf ./$PROJECT_DIR
  git clone $REMOTE $PROJECT_DIR
  cd $PROJECT_DIR
  git checkout $BRANCH
  rm -rf .git

  git checkout README.sentry.md
}

pull_premake_androidmk
