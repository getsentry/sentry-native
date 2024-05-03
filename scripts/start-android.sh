#!/usr/bin/env bash

# Adapted from:
# https://docs.microsoft.com/en-us/azure/devops/pipelines/ecosystems/android?view=azure-devops#test-on-the-android-emulator

echo "java version:"
java -version
echo "javac version"
javac -version
echo "PATH: $PATH"

ARCH=${ANDROID_ARCH:-"x86"}
API_LEVEL=${ANDROID_API:-"29"}
AVD_EMULATOR_NAME="sentry_android_${ARCH}"
IMAGE=${ANDROID_IMAGE:-"system-images;android-${API_LEVEL};google_apis;${ARCH}"}

# Create an Android Virtual Device
echo "Create Test AVDs with..."
echo "ARCH = $ARCH"
echo "API_LEVEL = $API_LEVEL"
echo "AVD_EMULATOR_NAME = $AVD_EMULATOR_NAME"
echo "IMAGE = $IMAGE"
echo "no" | $ANDROID_HOME/tools/bin/avdmanager create avd -n $AVD_EMULATOR_NAME -k "$IMAGE" --force

echo "List available AVDs..."
$ANDROID_HOME/emulator/emulator -list-avds

# Start emulator in background
echo "Starting emulator..."
nohup $ANDROID_HOME/emulator/emulator -avd $AVD_EMULATOR_NAME -no-snapshot > /dev/null 2>&1 &
echo "Wait for emulator availability..."
$ANDROID_HOME/platform-tools/adb wait-for-device shell 'ls'
echo "Verify emulator devices as running..."
$ANDROID_HOME/platform-tools/adb devices

echo "Emulator started."
