package io.sentry.ndk;

import static org.junit.Assert.assertEquals;

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
    // set tracesSampleRate to 1
    options.setTracesSampleRate(1);

    assertEquals(1.0f, options.getTracesSampleRate(), 0.0f);
  }

  @Test
  public void tracesSampleRateZero() {
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
    // set tracesSampleRate to 0
    options.setTracesSampleRate(0);

    assertEquals(0.0f, options.getTracesSampleRate(), 0.0f);
  }
}
