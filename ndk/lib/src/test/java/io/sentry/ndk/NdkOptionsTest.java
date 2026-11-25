package io.sentry.ndk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class NdkOptionsTest {

  @Test
  public void tracesSampleRate() {
    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            "out",
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk");
    options.setTracesSampleRate(1);

    assertEquals(1.0f, options.getTracesSampleRate(), 0.0f);
  }

  @Test
  public void tracesSampleRateDefaultsToZero() {
    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            "out",
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk");

    // default tracesSampleRate should be 0
    assertEquals(0.0f, options.getTracesSampleRate(), 0.0f);
  }

  @Test
  public void appHangAttachAllThreads() {
    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            "out",
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk");
    options.setAppHangAttachAllThreads(true);

    assertTrue(options.isAppHangAttachAllThreads());
  }

  @Test
  public void appHangAttachAllThreadsDefaultsToFalse() {
    final NdkOptions options =
        new NdkOptions(
            "https://key@sentry.io/proj",
            true,
            "out",
            "1.0.0",
            "production",
            "dist",
            100,
            "io.sentry.ndk");

    // by default only the hung thread is attached to app-hang events
    assertFalse(options.isAppHangAttachAllThreads());
  }
}
