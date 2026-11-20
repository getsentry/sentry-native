package io.sentry.e2e;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import io.sentry.Sentry;
import io.sentry.android.core.SentryAndroid;
import io.sentry.protocol.User;

public class MainActivity extends Activity {
  private static native void crash();

  @Override
  public void onCreate(Bundle state) {
    super.onCreate(state);
    init(getApplicationContext(), getIntent());
    if (getIntent().getBooleanExtra("crash", true)) {
      new Handler(getMainLooper()).postDelayed(MainActivity::triggerNativeCrash, 1000);
    }
  }

  static void init(Context context, Intent intent) {
    System.loadLibrary("e2e");
    SentryAndroid.init(
        context,
        options -> {
          options.setDsn(intent.getStringExtra("dsn"));
          options.setDebug(true);
          options.setRelease("sentry-native-e2e");
          options.setTombstoneEnabled(intent.getBooleanExtra("tombstone", false));
          options.setEnableAutoSessionTracking(false);
          options.setShutdownTimeoutMillis(30000);
        });
    String testId = intent.getStringExtra("test_id");
    Sentry.configureScope(
        scope -> {
          scope.setTag("test.id", testId);
          User user = new User();
          user.setId(testId);
          scope.setUser(user);
        });
  }

  private static void callNativeCrash() {
    crash();
  }

  static void triggerNativeCrash() {
    callNativeCrash();
  }
}
