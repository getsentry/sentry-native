package io.sentry.e2e;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import io.sentry.Sentry;

public class CrashReceiver extends BroadcastReceiver {
  @Override
  public void onReceive(Context context, Intent intent) {
    if (intent.hasExtra("dsn")) {
      MainActivity.init(context.getApplicationContext(), intent);
    }
    // a restarted process lost its crash service, so it must not crash
    if (!Sentry.isEnabled()) {
      return;
    }
    // keep the process active until the crash
    goAsync();
    new Handler(Looper.getMainLooper()).postDelayed(MainActivity::triggerNativeCrash, 1000);
  }
}
