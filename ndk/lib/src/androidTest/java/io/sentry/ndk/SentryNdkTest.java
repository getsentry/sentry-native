package io.sentry.ndk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.io.File;
import java.io.IOException;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;

import io.sentry.ndk.sample.NdkSample;

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
    public void transactionSampled() throws IOException {
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
        NdkSample.transaction();

        File[] files = outboxPath.listFiles();
        assertNotNull(files);
        assertEquals(1, files.length);
        // TODO check file contents and if it contains "type":"transaction"
    }

    @Test
    public void transactionNotSampled() throws IOException {
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
                        0.0f);

        // when initialized
        SentryNdk.init(options);

        // then it does not crash
        NdkSample.transaction();

        File[] files = outboxPath.listFiles();
        assertNotNull(files);
        assertEquals(0, files.length);
    }

  @Test
  public void shutdownDoesNotFailWhenNotInitialized() {
    // when closed without prior being initialized
    SentryNdk.close();

    // then it does not crash
  }
}
