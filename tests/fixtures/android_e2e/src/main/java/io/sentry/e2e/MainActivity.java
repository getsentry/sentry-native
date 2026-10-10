package io.sentry.e2e;

import android.app.Activity;
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
    System.loadLibrary("e2e");
    SentryAndroid.init(
        getApplicationContext(),
        options -> {
          options.setDsn(getIntent().getStringExtra("dsn"));
          options.setDebug(true);
          options.setRelease("sentry-native-e2e");
          options.setTombstoneEnabled(getIntent().getBooleanExtra("tombstone", false));
          options.setEnableAutoSessionTracking(false);
          options.setShutdownTimeoutMillis(30000);
        });
    String testId = getIntent().getStringExtra("test_id");
    Sentry.configureScope(
        scope -> {
          scope.setTag("test.id", testId);
          User user = new User();
          user.setId(testId);
          scope.setUser(user);
        });
    new Handler(getMainLooper()).postDelayed(MainActivity::triggerNativeCrash, 1000);
  }

  private static void callNativeCrash() {
    crash();
  }

  private static void triggerNativeCrash() {
    callNativeCrash();
  }
}
