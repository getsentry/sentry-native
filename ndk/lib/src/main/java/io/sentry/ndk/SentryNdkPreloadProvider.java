package io.sentry.ndk;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * Preloads the NDK integration before the Mono runtime. This installs and
 * chains the Native SDK's signal handlers before Mono, allowing Mono to
 * correctly handle managed exceptions while chaining native crashes to the
 * Native SDK.
 *
 * <p>Enabled by setting {@code io.sentry.ndk.preload} to {@code true} in
 * AndroidManifest.xml metadata. The high {@code initOrder} ensures this runs
 * before {@code mono.MonoRuntimeProvider}.
 */
public final class SentryNdkPreloadProvider extends ContentProvider {

  @Override
  public boolean onCreate() {
    final Context context = getContext();
    if (context == null) {
      return false;
    }
    try {
      final ApplicationInfo info =
          context
              .getPackageManager()
              .getApplicationInfo(context.getPackageName(), PackageManager.GET_META_DATA);
      final Bundle metadata = info.metaData;
      if (metadata != null && metadata.getBoolean("io.sentry.ndk.preload", false)) {
        android.util.Log.d("sentry", "io.sentry.ndk.preload read: true");
        SentryNdk.preload();
        android.util.Log.d("sentry", "SentryNdk.preload() completed");
      }
    } catch (Throwable e) {
      android.util.Log.e("sentry", "SentryNdk.preload() failed", e);
    }
    return true;
  }

  @Override
  public @Nullable Cursor query(
      @NotNull Uri uri,
      @Nullable String[] projection,
      @Nullable String selection,
      @Nullable String[] selectionArgs,
      @Nullable String sortOrder) {
    return null;
  }

  @Override
  public @Nullable String getType(@NotNull Uri uri) {
    return null;
  }

  @Override
  public @Nullable Uri insert(@NotNull Uri uri, @Nullable ContentValues values) {
    return null;
  }

  @Override
  public int delete(@NotNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
  }

  @Override
  public int update(
      @NotNull Uri uri,
      @Nullable ContentValues values,
      @Nullable String selection,
      @Nullable String[] selectionArgs) {
    return 0;
  }
}
