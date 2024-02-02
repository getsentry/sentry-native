package io.sentry.ndk;

import org.jetbrains.annotations.Nullable;

final class NativeModuleListLoader {

  public @Nullable DebugImage[] loadModuleList() {
    return nativeLoadModuleList();
  }

  public void clearModuleList() {
    nativeClearModuleList();
  }

  public static native DebugImage[] nativeLoadModuleList();

  public static native void nativeClearModuleList();
}
