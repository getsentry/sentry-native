package io.sentry.ndk;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.security.GeneralSecurityException;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManagerFactory;
import org.jetbrains.annotations.Nullable;

class SentryHttpClient {
  private final Object lock = new Object();
  private final ScheduledExecutorService timer =
      Executors.newSingleThreadScheduledExecutor(
          task -> {
            final Thread thread = new Thread(task, "sentry-http-timeout");
            thread.setDaemon(true);
            return thread;
          });
  private @Nullable HttpURLConnection connection;
  private boolean stopped;
  private long timeout;
  private String userAgent = "";
  private @Nullable Proxy proxy;
  private @Nullable SSLSocketFactory socketFactory;

  void configure(long timeout, @Nullable String proxy, @Nullable String caCerts, String userAgent)
      throws IOException, GeneralSecurityException {
    this.timeout = timeout;
    this.userAgent = userAgent;
    if (proxy != null) {
      final URL url = new URL(proxy.contains("://") ? proxy : "http://" + proxy);
      if (!"http".equals(url.getProtocol()) || url.getUserInfo() != null) {
        throw new IOException("Only unauthenticated HTTP proxies are supported");
      }
      this.proxy =
          new Proxy(
              Proxy.Type.HTTP,
              new InetSocketAddress(url.getHost(), url.getPort() < 0 ? 80 : url.getPort()));
    }
    if (caCerts != null) {
      final KeyStore store = KeyStore.getInstance(KeyStore.getDefaultType());
      store.load(null, null);
      try (InputStream input = new FileInputStream(caCerts)) {
        int index = 0;
        for (Certificate certificate :
            CertificateFactory.getInstance("X.509").generateCertificates(input)) {
          store.setCertificateEntry(Integer.toString(index++), certificate);
        }
      }
      final TrustManagerFactory factory =
          TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
      factory.init(store);
      final SSLContext context = SSLContext.getInstance("TLS");
      context.init(null, factory.getTrustManagers(), null);
      socketFactory = context.getSocketFactory();
    }
  }

  HttpURLConnection openConnection(URL url) throws IOException {
    return (HttpURLConnection) (proxy == null ? url.openConnection() : url.openConnection(proxy));
  }

  Response send(
      String method,
      String url,
      String[] headers,
      @Nullable ByteBuffer body,
      @Nullable String bodyPath,
      long bodyLength)
      throws IOException {
    final HttpURLConnection connection = openConnection(new URL(url));
    final AtomicBoolean expired = new AtomicBoolean();
    final @Nullable ScheduledFuture<?> deadline;
    synchronized (lock) {
      if (stopped) {
        connection.disconnect();
        throw new IOException("HTTP client is shut down");
      }
      this.connection = connection;
      deadline =
          timeout > 0
              ? timer.schedule(
                  () -> {
                    expired.set(true);
                    connection.disconnect();
                  },
                  timeout,
                  TimeUnit.MILLISECONDS)
              : null;
    }
    try {
      connection.setConnectTimeout((int) Math.min(timeout > 0 ? timeout : 15000, 15000));
      connection.setReadTimeout((int) Math.min(timeout, Integer.MAX_VALUE));
      connection.setUseCaches(false);
      connection.setInstanceFollowRedirects(false);
      connection.setRequestMethod(method);
      connection.setRequestProperty("User-Agent", userAgent);
      if (socketFactory != null && connection instanceof HttpsURLConnection) {
        ((HttpsURLConnection) connection).setSSLSocketFactory(socketFactory);
      }
      for (int i = 0; i < headers.length; i += 2) {
        connection.setRequestProperty(headers[i], headers[i + 1]);
      }
      checkCancelled(expired);
      final byte[] buffer = new byte[8192];
      if (body != null || bodyPath != null) {
        connection.setDoOutput(true);
        connection.setFixedLengthStreamingMode(bodyLength);
        try (OutputStream output = connection.getOutputStream()) {
          if (bodyPath != null) {
            try (InputStream input = new FileInputStream(bodyPath)) {
              int count;
              while ((count = input.read(buffer)) != -1) {
                checkCancelled(expired);
                output.write(buffer, 0, count);
              }
            }
          } else {
            while (body.hasRemaining()) {
              checkCancelled(expired);
              final int count = Math.min(body.remaining(), buffer.length);
              body.get(buffer, 0, count);
              output.write(buffer, 0, count);
            }
          }
        }
      }
      checkCancelled(expired);
      final int statusCode = connection.getResponseCode();
      if (statusCode < 0) {
        throw new IOException("Invalid HTTP response");
      }
      final List<String> responseHeaders = new ArrayList<>();
      for (Map.Entry<String, List<String>> header : connection.getHeaderFields().entrySet()) {
        if (header.getKey() != null) {
          for (String value : header.getValue()) {
            responseHeaders.add(header.getKey());
            responseHeaders.add(value);
          }
        }
      }
      try (InputStream input =
          statusCode >= 400 ? connection.getErrorStream() : connection.getInputStream()) {
        if (input != null) {
          while (input.read(buffer) != -1) {
            checkCancelled(expired);
          }
        }
      }
      checkCancelled(expired);
      return new Response(statusCode, responseHeaders.toArray(new String[0]));
    } finally {
      if (deadline != null) {
        deadline.cancel(false);
      }
      synchronized (lock) {
        this.connection = null;
      }
      connection.disconnect();
    }
  }

  private void checkCancelled(AtomicBoolean expired) throws IOException {
    if (expired.get()) {
      throw new SocketTimeoutException("HTTP request timed out");
    }
    synchronized (lock) {
      if (stopped) {
        throw new IOException("HTTP client is shut down");
      }
    }
  }

  void shutdown() {
    final @Nullable HttpURLConnection connection;
    synchronized (lock) {
      stopped = true;
      connection = this.connection;
    }
    timer.shutdownNow();
    if (connection != null) {
      connection.disconnect();
    }
  }

  static final class Response {
    final int statusCode;
    final String[] headers;

    Response(int statusCode, String[] headers) {
      this.statusCode = statusCode;
      this.headers = headers;
    }
  }
}
