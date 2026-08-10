#!/usr/bin/env bash

CFILES=$(git diff-index --cached --name-only --diff-filter=dr HEAD | grep -E "^(examples|include|src|tests/unit).*\.(c|h|cpp)$")
PYFILES=$(git diff-index --cached --name-only --diff-filter=dr HEAD | grep -E "^tests.*\.py$")

VENV_BIN=".venv/bin"
if [ "${OS:-}" = "Windows_NT" ]; then
    VENV_BIN=".venv/Scripts"
fi

if [ -n "$CFILES" ]; then
    "$VENV_BIN/clang-format" -i $CFILES
fi
if [ -n "$PYFILES" ]; then
    "$VENV_BIN/black" $PYFILES
fi
if [ -n "$CFILES$PYFILES" ]; then
    git add $CFILES $PYFILES
fi
