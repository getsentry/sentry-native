#!/usr/bin/env bash

ARCH="x86"
AVD_EMULATOR_NAME="sentry_android_${ARCH}"

# Adapted from:
# https://docs.microsoft.com/en-us/azure/devops/pipelines/ecosystems/android?view=azure-devops#test-on-the-android-emulator

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
