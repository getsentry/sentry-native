package io.sentry.ndk;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class SentryHttpClientTest {
  private Connection connection;
  private SentryHttpClient client;

  @Before
  public void setUp() throws Exception {
    connection = new Connection();
    client =
        new SentryHttpClient() {
          @Override
          HttpURLConnection openConnection(URL url) {
            return connection;
          }
        };
    client.configure(5000, null, null, "sentry-native/test");
  }

  @After
  public void tearDown() {
    client.shutdown();
  }

  @Test
  public void sendsBinaryEnvelope() throws Exception {
    final byte[] body = {0, 1, (byte) 0xff, 0, 42};
    final SentryHttpClient.Response response =
        client.send(
            "POST",
            connection.getURL().toString(),
            new String[] {"Content-Type", "application/x-sentry-envelope", "X-Sentry-Auth", "auth"},
            ByteBuffer.wrap(body),
            null,
            body.length);

    assertEquals(200, response.statusCode);
    assertEquals("POST", connection.getRequestMethod());
    assertEquals("auth", connection.getRequestProperty("X-Sentry-Auth"));
    assertEquals("sentry-native/test", connection.getRequestProperty("User-Agent"));
    assertEquals("application/x-sentry-envelope", connection.getRequestProperty("Content-Type"));
    assertEquals(body.length, connection.bodyLength());
    assertArrayEquals(body, connection.output.toByteArray());
    assertFalse(connection.getUseCaches());
    assertFalse(connection.getInstanceFollowRedirects());
    assertTrue(connection.disconnected);
  }

  @Test
  public void streamsAttachment() throws Exception {
    final File file = File.createTempFile("sentry-http", ".attachment");
    try {
      final byte[] body = new byte[20000];
      Arrays.fill(body, (byte) 0xff);
      try (OutputStream output = new FileOutputStream(file)) {
        output.write(body);
      }
      client.send(
          "PATCH",
          connection.getURL().toString(),
          new String[] {"Tus-Resumable", "1.0.0"},
          null,
          file.getPath(),
          body.length);

      assertEquals("PATCH", connection.getRequestMethod());
      assertEquals(body.length, connection.bodyLength());
      assertArrayEquals(body, connection.output.toByteArray());
    } finally {
      assertTrue(file.delete());
    }
  }

  @Test
  public void sendsBodylessRequest() throws Exception {
    client.send("POST", connection.getURL().toString(), new String[0], null, null, 0);

    assertEquals("POST", connection.getRequestMethod());
    assertFalse(connection.getDoOutput());
    assertEquals(0, connection.output.size());
  }

  @Test
  public void returnsErrorStatusAndHeaders() throws Exception {
    connection.statusCode = 429;
    connection.headers.put(null, Collections.singletonList("HTTP/1.1 429 Too Many Requests"));
    connection.headers.put("Retry-After", Collections.singletonList("60"));
    connection.headers.put("X-Sentry-Rate-Limits", Collections.singletonList("60:error:key"));
    connection.headers.put("Location", Collections.singletonList("/upload/1"));
    connection.headers.put("Other", Arrays.asList("one", "two"));

    final SentryHttpClient.Response response =
        client.send("POST", connection.getURL().toString(), new String[0], null, null, 0);

    assertEquals(429, response.statusCode);
    assertArrayEquals(
        new String[] {
          "Retry-After",
          "60",
          "X-Sentry-Rate-Limits",
          "60:error:key",
          "Location",
          "/upload/1",
          "Other",
          "one",
          "Other",
          "two"
        },
        response.headers);
  }

  @Test
  public void disconnectsOnFailure() {
    connection.failure = new IOException("network unavailable");

    assertThrows(
        IOException.class,
        () -> client.send("POST", connection.getURL().toString(), new String[0], null, null, 0));

    assertTrue(connection.disconnected);
  }

  @Test
  public void cancelsTimedOutRequest() throws Exception {
    client.configure(20, null, null, "test");
    connection.block = true;

    assertThrows(
        IOException.class,
        () -> client.send("POST", connection.getURL().toString(), new String[0], null, null, 0));

    assertTrue(connection.disconnected);
  }

  @Test
  public void shutdownCancelsRequest() throws Exception {
    connection.block = true;
    final ExecutorService worker = Executors.newSingleThreadExecutor();
    try {
      final Future<IOException> failure =
          worker.submit(
              () ->
                  assertThrows(
                      IOException.class,
                      () ->
                          client.send(
                              "POST",
                              connection.getURL().toString(),
                              new String[0],
                              null,
                              null,
                              0)));
      assertTrue(connection.entered.await(2, TimeUnit.SECONDS));
      client.shutdown();

      assertNotNull(failure.get(2, TimeUnit.SECONDS));
      assertTrue(connection.disconnected);
    } finally {
      worker.shutdownNow();
    }
  }

  @Test
  public void rejectsRequestsAfterShutdown() {
    client.shutdown();

    assertThrows(
        IOException.class,
        () -> client.send("POST", connection.getURL().toString(), new String[0], null, null, 0));
  }

  @Test
  public void rejectsInvalidResponse() {
    connection.statusCode = -1;

    assertThrows(
        IOException.class,
        () -> client.send("POST", connection.getURL().toString(), new String[0], null, null, 0));

    assertTrue(connection.disconnected);
  }

  @Test
  public void shutdownBeforeConnectPreventsUpload() {
    connection.beforeRequest = client::shutdown;

    assertThrows(
        IOException.class,
        () ->
            client.send(
                "POST",
                connection.getURL().toString(),
                new String[0],
                ByteBuffer.wrap(new byte[] {1}),
                null,
                1));

    assertEquals(0, connection.output.size());
  }

  private static final class Connection extends HttpURLConnection {
    final ByteArrayOutputStream output = new ByteArrayOutputStream();
    final Map<String, List<String>> headers = new LinkedHashMap<>();
    final CountDownLatch entered = new CountDownLatch(1);
    final CountDownLatch cancelled = new CountDownLatch(1);
    volatile boolean disconnected;
    boolean block;
    int statusCode = 200;
    IOException failure;
    Runnable beforeRequest;

    Connection() throws Exception {
      super(new URL("http://localhost/envelope/"));
    }

    long bodyLength() {
      return fixedContentLengthLong;
    }

    @Override
    public void setRequestMethod(String method) {
      this.method = method;
      if (beforeRequest != null) {
        beforeRequest.run();
      }
    }

    @Override
    public OutputStream getOutputStream() {
      return output;
    }

    @Override
    public int getResponseCode() throws IOException {
      if (failure != null) {
        throw failure;
      }
      if (block) {
        entered.countDown();
        try {
          assertTrue(cancelled.await(2, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        }
        throw new IOException("cancelled");
      }
      return statusCode;
    }

    @Override
    public Map<String, List<String>> getHeaderFields() {
      return headers;
    }

    @Override
    public InputStream getInputStream() {
      return new ByteArrayInputStream(new byte[0]);
    }

    @Override
    public InputStream getErrorStream() {
      return null;
    }

    @Override
    public void disconnect() {
      disconnected = true;
      cancelled.countDown();
    }

    @Override
    public boolean usingProxy() {
      return false;
    }

    @Override
    public void connect() {}
  }
}
