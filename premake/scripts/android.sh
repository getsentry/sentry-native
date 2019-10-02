#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR/..

if [ -z "${ANDROID_HOME:-}" ]; then
    echo "ANDROID_HOME is not set. Please point it to your Android SDK directory."
fi

if [ -n "${JAVA8_HOME:-}" ]; then
    export JAVA_HOME=${JAVA8_HOME:-}
fi

ARCH="x86"
AVD_EMULATOR_NAME="android_${ARCH}"


start_emulator() {
    # Install AVD files
    echo "y" | $ANDROID_HOME/tools/bin/sdkmanager --install "system-images;android-27;google_apis;${ARCH}"

    # Create an Android Virtual Device
    echo "no" | $ANDROID_HOME/tools/bin/avdmanager create avd -n $AVD_EMULATOR_NAME -k 'system-images;android-27;google_apis;x86' --force

    $ANDROID_HOME/emulator/emulator -list-avds

    echo "Starting emulator..."

    # Start emulator in background
    nohup $ANDROID_HOME/emulator/emulator -avd $AVD_EMULATOR_NAME -no-snapshot > /dev/null 2>&1 &
    $ANDROID_HOME/platform-tools/adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed | tr -d '\r') ]]; do sleep 1; done; input keyevent 82'
    $ANDROID_HOME/platform-tools/adb devices

    echo "Emulator started."
}


is_emulator_running() {
    if [ -n "$($ANDROID_HOME/platform-tools/adb devices | grep "emulator-" | cut -f1)" ]; then
        true
    else
        false
    fi
}


stop_all_emulators() {
    echo "Stopping all emulators..."
    $ANDROID_HOME/platform-tools/adb devices | grep "emulator-" | cut -f1 | while read line; do adb -s $line emu kill; done
    echo "Done."
}


run_tests() {
    if is_emulator_running; then
        echo "Emulator is already running"
    else
        start_emulator
    fi

    DEVICE_DIR="/data/local/tmp"
    $ANDROID_HOME/platform-tools/adb push libs/ "${DEVICE_DIR}"
    $ANDROID_HOME/platform-tools/adb shell "${DEVICE_DIR}/libs/${ARCH}/test_sentry"
}

show_help() {
    echo "
Commands:
    run-tests -- run tests in an emulator
    stop-emulators -- stop all running emulators
"
}

COMMAND="${1:-}"
if [ "${COMMAND}" == "run-tests" ]; then
    run_tests
elif [ "${COMMAND}" == "stop-emulators" ]; then
    stop_all_emulators
else
    echo "Invalid command"
    show_help
    exit 1
fi
