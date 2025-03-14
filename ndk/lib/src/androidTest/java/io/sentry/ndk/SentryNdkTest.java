package io.sentry.ndk;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.io.File;
import java.io.IOException;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SentryNdkTest {

  @Test
  public void initDoesNotFail() throws IOException {
    final TemporaryFolder temporaryFolder = TemporaryFolder.builder().build();
    temporaryFolder.create();
    final File outboxPath = temporaryFolder.newFolder("outboxPath");

    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            outboxPath.getAbsolutePath(),
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk",
            1.0f);

    // when initialized
    SentryNdk.init(options);

    // then it does not crash
  }

  @Test
  public void shutdownDoesNotFail() throws IOException {
    final TemporaryFolder temporaryFolder = TemporaryFolder.builder().build();
    temporaryFolder.create();
    final File outboxPath = temporaryFolder.newFolder("outboxPath");

    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            outboxPath.getAbsolutePath(),
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk",
            1.0f);

    // when initialized
    SentryNdk.init(options);

    // and then closed
    SentryNdk.close();

    // it does not crash
  }

  @Test
  public void shutdownDoesNotFailWhenNotInitialized() {
    // when closed without prior being initialized
    SentryNdk.close();

    // then it does not crash
  }
}
