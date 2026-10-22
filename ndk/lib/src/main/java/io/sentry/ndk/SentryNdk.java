package io.sentry.ndk;

import org.jetbrains.annotations.ApiStatus;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

@ApiStatus.Internal
public final class SentryNdk {

  private static volatile boolean nativeLibrariesLoaded;

  private SentryNdk() {}

  private static native void preloadSentryNative();

  /**
   * Initializes sentry-native and returns 0 on success, non-zero on failure.
   *
   * @return -1 if an JNI or options configuration issue occurred, 1 if sentry native itself failed
   *     to initialize
   */
  private static native int initSentryNative(@NotNull final NdkOptions options);

  private static native void nativeInitCrashDaemon(
      @NotNull String shmPath, int notifyFd, int readyFd, int messageFd);

  private static native long nativeRunCrashDaemon(
      int appPid, long appTid, int notifyFd, int readyFd, int messageFd, @NotNull String shmPath);

  private static native @NotNull String nativeGetCrashEnvelopePath(long handle);

  private static native int nativeSendEnvelope(long handle, @NotNull String path, long timeout);

  private static native void nativeCloseCrashDaemon(long handle);

  private static native void shutdown();

  /**
   * Preloads sentry-native into the process signal chain before full initialization.
   *
   * <p>Intended for downstream SDK/runtime integrations on Android. This does not initialize
   * sentry-native, configure a database path, or start the inproc handler thread; it only
   * establishes signal-handler ordering ahead of the managed runtime.
   *
   * <p>This is intended to be used by downstream integrations that gate the preload flow to
   * supported runtimes and then call {@link #init(NdkOptions)} with the normal DEFAULT handler
   * strategy.
   */
  public static void preload() {
    loadNativeLibraries();
    preloadSentryNative();
  }

  /**
   * Init the NDK integration
   *
   * @param options the SentryAndroidOptions
   * @throws IllegalStateException if sentry-native couldn't be initialized
   */
  public static void init(@NotNull final NdkOptions options) {
    loadNativeLibraries();
    final int returnCode = initSentryNative(options);
    if (returnCode > 0) {
      throw new IllegalStateException(
          "A sentry-native internal init error occurred, please check the logs for more details.");
    } else if (returnCode < 0) {
      throw new IllegalStateException("A sentry-native setup failure occurred");
    }
  }

  public static void initCrashDaemon(
      @NotNull String shmPath, int notifyFd, int readyFd, int messageFd) {
    loadNativeLibraries();
    nativeInitCrashDaemon(shmPath, notifyFd, readyFd, messageFd);
  }

  public static @Nullable CrashDaemon runCrashDaemon(
      int appPid, long appTid, int notifyFd, int readyFd, int messageFd, @NotNull String shmPath) {
    loadNativeLibraries();
    final long handle = nativeRunCrashDaemon(appPid, appTid, notifyFd, readyFd, messageFd, shmPath);
    return handle == 0 ? null : new CrashDaemon(handle);
  }

  @ApiStatus.Internal
  public static final class CrashDaemon implements AutoCloseable {
    private final @NotNull String envelopePath;
    private long handle;

    private CrashDaemon(long handle) {
      this.handle = handle;
      envelopePath = nativeGetCrashEnvelopePath(handle);
    }

    public @NotNull String getEnvelopePath() {
      return envelopePath;
    }

    /** Sends the merged envelope and closes the daemon. */
    public synchronized @NotNull SendResult sendEnvelope(@NotNull String path, long timeout) {
      final long handle = this.handle;
      this.handle = 0;
      return handle != 0
          ? SendResult.values()[nativeSendEnvelope(handle, path, timeout)]
          : SendResult.FAILED;
    }

    @Override
    public synchronized void close() {
      final long handle = this.handle;
      this.handle = 0;
      if (handle != 0) {
        nativeCloseCrashDaemon(handle);
      }
    }

    /** The outcome of {@link #sendEnvelope(String, long)}. */
    public enum SendResult {
      /** The envelope was neither sent nor kept. */
      FAILED,
      /** The envelope was sent. */
      SENT,
      /** The envelope was not sent and is kept for the next launch. */
      KEPT
    }
  }

  /** Closes the NDK integration */
  public static void close() {
    loadNativeLibraries();
    shutdown();
  }

  /**
   * Loads all required native libraries. This is automatically done by {@link #init(NdkOptions)},
   * but can be called manually in case you want to preload the libraries before calling #init.
   */
  public static synchronized void loadNativeLibraries() {
    if (!nativeLibrariesLoaded) {
      // On older Android versions, it was necessary to manually call "`System.loadLibrary` on all
      // transitive dependencies before loading [the] main library."
      // The dependencies of `libsentry.so` are currently `lib{c,m,dl,log}.so`.
      // See
      // https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#changes-to-library-dependency-resolution
      System.loadLibrary("log");
      System.loadLibrary("sentry");
      System.loadLibrary("sentry-android");
      nativeLibrariesLoaded = true;
    }
  }
}
