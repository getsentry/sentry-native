package io.sentry.ndk;

import org.jetbrains.annotations.ApiStatus;
import org.jetbrains.annotations.NotNull;

@ApiStatus.Internal
public final class SentryNdk {

    static {
        // On older Android versions, it was necessary to manually call "`System.loadLibrary` on all
        // transitive dependencies before loading [the] main library."
        // The dependencies of `libsentry.so` are currently `lib{c,m,dl,log}.so`.
        // See
        // https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#changes-to-library-dependency-resolution
        System.loadLibrary("log");
        System.loadLibrary("sentry");
        System.loadLibrary("sentry-android");
    }

    private SentryNdk() {
    }

    private static native void initSentryNative(@NotNull final NdkOptions options);

    private static native void shutdown();

    /**
     * Init the NDK integration
     *
     * @param options the SentryAndroidOptions
     */
    public static void init(@NotNull final NdkOptions options) {
        initSentryNative(options);
    }

    /**
     * Closes the NDK integration
     */
    public static void close() {
        shutdown();
    }
}
