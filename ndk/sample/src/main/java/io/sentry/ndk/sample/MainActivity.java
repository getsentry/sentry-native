package io.sentry.ndk.sample;

import android.app.Activity;
import android.os.Bundle;
import io.sentry.Integration;
import io.sentry.SentryLevel;
import io.sentry.android.core.SentryAndroid;
import io.sentry.ndk.NdkOptions;
import io.sentry.ndk.SentryNdk;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

public class MainActivity extends Activity {

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    // Utilize Android SDK merely as a transport for sending outbox data
    final AtomicReference<File> outbox = new AtomicReference<>();
    SentryAndroid.init(
        this,
        options -> {
          options.setDsn(
              "https://d545f4da00ff7b2fa7b6c8620c94a4e9@o447951.ingest.us.sentry.io/4506178389999616");
          final List<Integration> integrations = options.getIntegrations();
          final List<Integration> usedIntegrations = new ArrayList<>();
          for (Integration integration : integrations) {
            final String name = integration.getClass().getSimpleName();
            if ("SendCachedEnvelopeIntegration".equals(name) || name.contains("Outbox")) {
              usedIntegrations.add(integration);
            }
          }
          integrations.clear();
          integrations.addAll(usedIntegrations);

          options.setDiagnosticLevel(SentryLevel.DEBUG);
          options.setDebug(true);
          outbox.set(new File(options.getOutboxPath()));
        });

    findViewById(R.id.init_ndk_button).setOnClickListener(v -> initNdk(outbox.get()));
    findViewById(R.id.trigger_native_crash_button).setOnClickListener(v -> NdkSample.crash());
    findViewById(R.id.capture_message_button).setOnClickListener(v -> NdkSample.message());
    findViewById(R.id.capture_transaction_button).setOnClickListener(v -> NdkSample.transaction());
  }

  private void initNdk(final File outboxFolder) {
    setupOutboxFolder(outboxFolder);
    final NdkOptions options =
        new NdkOptions(
            "https://d545f4da00ff7b2fa7b6c8620c94a4e9@o447951.ingest.us.sentry.io/4506178389999616",
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

  private void setupOutboxFolder(File outboxDir) {
    // ensure we have a proper outbox directory
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
  }
}
