package io.sentry.ndk.sample;

import android.app.Activity;
import android.os.Bundle;
import io.sentry.ndk.NdkOptions;
import io.sentry.ndk.SentryNdk;
import java.io.File;

public class MainActivity extends Activity {
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    findViewById(R.id.init_ndk_button).setOnClickListener(v -> initNdk());
    findViewById(R.id.trigger_native_crash_button).setOnClickListener(v -> NdkSample.crash());
    findViewById(R.id.capture_message_button).setOnClickListener(v -> NdkSample.message());
    findViewById(R.id.capture_transaction_button).setOnClickListener(v -> NdkSample.transaction());
  }

  private void initNdk() {
    // The outbox directory is created by sentry-native, so we only need to
    // hand it the path here.
    final File outboxFolder = new File(getFilesDir(), "outbox");
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
}
