#!/usr/bin/env bash
set -euxo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sanitizer="${1:-}"
[[ -n "$sanitizer" ]] || exit 0

: "${ANDROID_HOME:?ANDROID_HOME must point to the Android SDK}"
: "${ANDROID_NDK:?ANDROID_NDK must name an installed NDK version}"
: "${ANDROID_ARCH:?ANDROID_ARCH must name the emulator ABI}"

if [[ "$sanitizer" != asan && "$sanitizer" != tsan ]]; then
    echo "Unsupported sanitizer: $sanitizer" >&2
    exit 1
fi

case "$ANDROID_ARCH" in
    armeabi-v7a) runtime_arch=arm ;;
    arm64-v8a) runtime_arch=aarch64 ;;
    x86) runtime_arch=i686 ;;
    x86_64) runtime_arch=x86_64 ;;
    *)
        echo "Unsupported Android ABI: $ANDROID_ARCH" >&2
        exit 1
        ;;
esac

ndk_dir="$ANDROID_HOME/ndk/$ANDROID_NDK"
runtime=$(find "$ndk_dir/toolchains/llvm/prebuilt" \
    -name "libclang_rt.$sanitizer-$runtime_arch-android.so" -print -quit)
if [[ ! -f "$runtime" ]]; then
    echo "No $sanitizer runtime for Android ABI: $ANDROID_ARCH" >&2
    exit 1
fi

sanitizer_dir="$script_dir/../ndk/lib/build/$sanitizer"
runtime_dir="$sanitizer_dir/jniLibs/$ANDROID_ARCH"
resource_dir="$sanitizer_dir/resources/lib/$ANDROID_ARCH"

"$ANDROID_HOME/platform-tools/adb" push "$runtime" /data/local/tmp/
mkdir -p "$runtime_dir"
cp "$runtime" "$runtime_dir/"

wrapper="$ndk_dir/wrap.sh/$sanitizer.sh"
if [[ -f "$wrapper" ]]; then
    mkdir -p "$resource_dir"
    cp "$wrapper" "$resource_dir/wrap.sh"
fi
