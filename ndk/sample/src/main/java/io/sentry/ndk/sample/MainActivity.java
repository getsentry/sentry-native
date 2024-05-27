package io.sentry.ndk.sample;

import android.app.Activity;
import android.os.Bundle;

import java.io.File;

import io.sentry.ndk.NdkOptions;
import io.sentry.ndk.SentryNdk;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.init_ndk_button).setOnClickListener(v -> initNdk());
        findViewById(R.id.trigger_native_crash_button).setOnClickListener(v -> NdkSample.crash());
        findViewById(R.id.capture_message_button).setOnClickListener(v -> NdkSample.message());
    }

    private void initNdk() {
        final File outboxFolder = setupOutboxFolder();
        final NdkOptions options = new NdkOptions(
                "https://1053864c67cc410aa1ffc9701bd6f93d@o447951.ingest.sentry.io/5428559",
                BuildConfig.DEBUG,
                outboxFolder.getAbsolutePath(),
                "1.0.0",
                "production",
                BuildConfig.VERSION_NAME,
                100,
                "sentry-native-jni"
        );
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
