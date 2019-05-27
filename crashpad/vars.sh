#!/usr/bin/env bash
set -eux

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export PATH="$SCRIPT_DIR/build/depot_tools:$PATH"

if [[ "${AGENT_OS:-}" == "Windows_NT" ]]; then
  FETCH_CMD='fetch.bat'
  GN_CMD='gn.bat'
  NINJA_CMD='ninja.exe'
  GCLIENT_CMD='gclient.bat'
else
  FETCH_CMD='fetch'
  GN_CMD='gn'
  NINJA_CMD='ninja'
  GCLIENT_CMD='gclient'
fi
