package io.sentry.ndk.sample;

import android.app.Activity;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;
import android.widget.Toast;
import io.sentry.ndk.NdkOptions;
import io.sentry.ndk.SentryNdk;
import java.io.File;

public class MainActivity extends Activity {
  private static final String TAG = "NdkSample";

  private int mainTid;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    mainTid = Process.myTid();

    findViewById(R.id.init_ndk_button).setOnClickListener(v -> initNdk());
    findViewById(R.id.trigger_native_crash_button).setOnClickListener(v -> NdkSample.crash());
    findViewById(R.id.capture_message_button).setOnClickListener(v -> NdkSample.message());
    findViewById(R.id.capture_transaction_button).setOnClickListener(v -> NdkSample.transaction());
    findViewById(R.id.capture_main_stack_button).setOnClickListener(v -> captureMainStack());
  }

  /**
   * Sample the main thread's native stack from a background thread. Mirrors how an ANR watchdog
   * would call into the NDK after detecting that the UI thread is stuck.
   */
  private void captureMainStack() {
    final int tid = mainTid;
    new Thread(
            () -> {
              final long[] frames = SentryNdk.captureThreadStack(tid);
              Log.i(TAG, "Captured " + frames.length + " frames from main thread (tid=" + tid + ")");
              for (int i = 0; i < frames.length; i++) {
                Log.i(TAG, String.format("  #%02d: 0x%016x", i, frames[i]));
              }
              runOnUiThread(
                  () ->
                      Toast.makeText(
                              this,
                              "Captured " + frames.length + " frames (see logcat)",
                              Toast.LENGTH_SHORT)
                          .show());
            },
            "ndk-sample-sampler")
        .start();
  }

  private void initNdk() {
    final File outboxFolder = setupOutboxFolder();
    final NdkOptions options =
        new NdkOptions(
            "https://1053864c67cc410aa1ffc9701bd6f93d@o447951.ingest.sentry.io/5428559",
            BuildConfig.DEBUG,
            outboxFolder.getAbsolutePath(),
            "1.0.0",
            "production",
            BuildConfig.VERSION_NAME,
            100,
            "sentry-native-jni");
    // set tracesSampleRate to 1
    options.setTracesSampleRate(1);
    SentryNdk.init(options);
  }

  private File setupOutboxFolder() {
    // ensure we have a proper outbox directory
    final File outboxDir = new File(getFilesDir(), "outbox");
    if (outboxDir.isFile()) {
      final boolean deleteOk = outboxDir.delete();
      if (!deleteOk) {
        throw new IllegalStateException("Failed to delete outbox file: " + outboxDir);
      }
    }
    if (!outboxDir.exists()) {
      final boolean mkdirOk = outboxDir.mkdirs();
      if (!mkdirOk) {
        throw new IllegalStateException("Failed to create outbox directory: " + outboxDir);
      }
    }
    return outboxDir;
  }
}
