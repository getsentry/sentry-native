#include "sentry_http_transport.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_utils.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#if !__has_feature(objc_arc)
#    error "the apple transport requires ARC (-fobjc-arc)"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

@interface SentryURLSessionClient : NSObject <NSURLSessionTaskDelegate>
@property (nonatomic, strong) NSURLSession *session;
@property (nonatomic, strong) NSURLSessionTask *task;
@property (nonatomic, strong) NSString *userAgent;
@property (nonatomic, strong) NSArray *anchors;
@property (nonatomic, strong) NSURLCredential *proxyCredential;
@property (nonatomic, strong) NSString *proxyAuthorization;
@property (nonatomic) BOOL shutdown;
@end

@implementation SentryURLSessionClient

- (void)URLSession:(NSURLSession *)session
                   task:(NSURLSessionTask *)task
    didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
      completionHandler:
          (void (^)(NSURLSessionAuthChallengeDisposition disposition,
              NSURLCredential *credential))completionHandler
{
    (void)session;
    (void)task;
    NSURLProtectionSpace *space = challenge.protectionSpace;
    if (self.anchors &&
        [space.authenticationMethod
            isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        // like `CURLOPT_CAINFO`, trust only the given certificates
        SecTrustRef trust = space.serverTrust;
        SecTrustSetAnchorCertificates(trust, (__bridge CFArrayRef)self.anchors);
        SecTrustSetAnchorCertificatesOnly(trust, true);
        CFErrorRef error = NULL;
        if (SecTrustEvaluateWithError(trust, &error)) {
            completionHandler(NSURLSessionAuthChallengeUseCredential,
                [NSURLCredential credentialForTrust:trust]);
        } else {
            NSError *err = (__bridge_transfer NSError *)error;
            SENTRY_WARNF("server certificate not trusted: %s",
                err.localizedDescription.UTF8String);
            completionHandler(
                NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
        }
        return;
    }
    // answer the proxy's 407 to the CONNECT of an HTTPS tunnel
    if (space.isProxy && self.proxyCredential
        && challenge.previousFailureCount == 0) {
        completionHandler(
            NSURLSessionAuthChallengeUseCredential, self.proxyCredential);
        return;
    }
    completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

@end

static NSArray *
load_ca_certs(const char *path)
{
    NSMutableArray *certs = [NSMutableArray array];
    NSString *pem = [NSString stringWithContentsOfFile:@(path)
                                              encoding:NSASCIIStringEncoding
                                                 error:nil];
    if (!pem) {
        SENTRY_WARNF("failed to read CA certificates \"%s\"", path);
        return certs;
    }
    NSString *begin = @"-----BEGIN CERTIFICATE-----";
    NSString *end = @"-----END CERTIFICATE-----";
    NSRange range = NSMakeRange(0, pem.length);
    while (true) {
        NSRange start = [pem rangeOfString:begin options:0 range:range];
        if (start.location == NSNotFound) {
            break;
        }
        NSUInteger body = NSMaxRange(start);
        NSRange stop = [pem rangeOfString:end
                                  options:0
                                    range:NSMakeRange(body, pem.length - body)];
        if (stop.location == NSNotFound) {
            break;
        }
        NSString *base64 =
            [pem substringWithRange:NSMakeRange(body, stop.location - body)];
        NSData *der = [[NSData alloc]
            initWithBase64EncodedString:base64
                                options:
                                    NSDataBase64DecodingIgnoreUnknownCharacters];
        SecCertificateRef cert = der
            ? SecCertificateCreateWithData(NULL, (__bridge CFDataRef)der)
            : NULL;
        if (cert) {
            [certs addObject:(__bridge_transfer id)cert];
        }
        NSUInteger next = NSMaxRange(stop);
        range = NSMakeRange(next, pem.length - next);
    }
    if (certs.count == 0) {
        SENTRY_WARNF("no CA certificates found in \"%s\"", path);
    }
    return certs;
}

static void
configure_proxy(SentryURLSessionClient *client,
    NSURLSessionConfiguration *config, const char *proxy)
{
    // an empty dictionary disables the proxy, nil uses the system settings
    if (!proxy || !proxy[0]) {
        config.connectionProxyDictionary
            = proxy ? [NSDictionary dictionary] : nil;
        return;
    }

    sentry_url_t url;
    if (sentry__url_parse(&url, proxy, false) != 0) {
        SENTRY_WARNF("invalid proxy \"%s\"", proxy);
        return;
    }

    NSString *host = @(url.host);
    if ([host hasPrefix:@"["] && [host hasSuffix:@"]"]) {
        host = [host substringWithRange:NSMakeRange(1, host.length - 2)];
    }

    NSDictionary *dict = nil;
    if (sentry__string_eq(url.scheme, "http")
        || sentry__string_eq(url.scheme, "https")) {
        // the keys refer to the target URL, not to the proxy URL
        NSNumber *port = @(url.port);
        dict = @{
            @"HTTPEnable" : @YES,
            @"HTTPProxy" : host,
            @"HTTPPort" : port,
            @"HTTPSEnable" : @YES,
            @"HTTPSProxy" : host,
            @"HTTPSPort" : port,
        };
        if (url.username) {
            NSString *user = @(url.username);
            NSString *password = @(url.password ? url.password : "");
            client.proxyCredential = [NSURLCredential
                credentialWithUser:user
                          password:password
                       persistence:NSURLCredentialPersistenceNone];
            NSString *credentials =
                [NSString stringWithFormat:@"%@:%@", user, password];
            NSString *basic =
                [[credentials dataUsingEncoding:NSUTF8StringEncoding]
                    base64EncodedStringWithOptions:0];
            client.proxyAuthorization =
                [NSString stringWithFormat:@"Basic %@", basic];
        }
    } else if (sentry__string_eq(url.scheme, "socks5")
        || sentry__string_eq(url.scheme, "socks5h")) {
        dict = @{
            @"SOCKSEnable" : @YES,
            @"SOCKSProxy" : host,
            @"SOCKSPort" : @(url.port ? url.port : 1080),
        };
        if (url.username) {
            SENTRY_WARN("SOCKS proxy credentials are not supported");
        }
    } else {
        SENTRY_WARNF("unsupported proxy scheme \"%s\"", url.scheme);
    }
    config.connectionProxyDictionary = dict;

    sentry__url_cleanup(&url);
}

// streams `length` bytes of `file` instead of reading them into memory, like
// crashpad's `CrashpadHTTPBodyStreamTransport`
@interface SentryFileBodyStream : NSInputStream
- (instancetype)initWithFile:(FILE *)file length:(size_t)length;
@end

@implementation SentryFileBodyStream {
    FILE *_file;
    size_t _remaining;
    NSStreamStatus _status;
    __weak id<NSStreamDelegate> _delegate;
}

- (instancetype)initWithFile:(FILE *)file length:(size_t)length
{
    if (!(self = [super init])) {
        fclose(file);
        return nil;
    }
    _file = file;
    _remaining = length;
    _status = NSStreamStatusNotOpen;
    return self;
}

- (void)dealloc
{
    fclose(_file);
}

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len
{
    if (len > _remaining) {
        len = _remaining;
    }
    if (fread(buffer, 1, len, _file) != len) {
        _status = NSStreamStatusError;
        return -1;
    }
    _remaining -= len;
    _status = _remaining ? NSStreamStatusOpen : NSStreamStatusAtEnd;
    return (NSInteger)len;
}

- (BOOL)getBuffer:(uint8_t **)buffer length:(NSUInteger *)len
{
    (void)buffer;
    (void)len;
    return NO;
}

- (BOOL)hasBytesAvailable
{
    return _status == NSStreamStatusOpen;
}

- (void)open
{
    _status = NSStreamStatusOpen;
}

- (void)close
{
    _status = NSStreamStatusClosed;
}

- (NSStreamStatus)streamStatus
{
    return _status;
}

- (NSError *)streamError
{
    return _status == NSStreamStatusError
        ? [NSError errorWithDomain:NSPOSIXErrorDomain code:EIO userInfo:nil]
        : nil;
}

- (id<NSStreamDelegate>)delegate
{
    return _delegate;
}

- (void)setDelegate:(id<NSStreamDelegate>)delegate
{
    _delegate = delegate;
}

- (id)propertyForKey:(NSStreamPropertyKey)key
{
    (void)key;
    return nil;
}

- (BOOL)setProperty:(id)property forKey:(NSStreamPropertyKey)key
{
    (void)property;
    (void)key;
    return NO;
}

- (void)scheduleInRunLoop:(NSRunLoop *)loop forMode:(NSRunLoopMode)mode
{
    (void)loop;
    (void)mode;
}

- (void)removeFromRunLoop:(NSRunLoop *)loop forMode:(NSRunLoopMode)mode
{
    (void)loop;
    (void)mode;
}

@end

static int
apple_client_start(void *_client, const sentry_options_t *options)
{
    SentryURLSessionClient *client = (__bridge SentryURLSessionClient *)_client;
    @autoreleasepool {
        NSURLSessionConfiguration *config =
            [NSURLSessionConfiguration ephemeralSessionConfiguration];
        if (options->transfer_timeout) {
            config.timeoutIntervalForResource
                = (NSTimeInterval)options->transfer_timeout / 1000.0;
        }

        const char *proxy = options->proxy;
        if (!proxy && options->dsn) {
            proxy = getenv(
                options->dsn->is_secure ? "https_proxy" : "http_proxy");
        }
        configure_proxy(client, config, proxy);

        if (options->ca_certs) {
            client.anchors = load_ca_certs(options->ca_certs);
        }
        if (options->user_agent) {
            client.userAgent = @(options->user_agent);
        }

        client.session = [NSURLSession sessionWithConfiguration:config
                                                       delegate:client
                                                  delegateQueue:nil];
    }
    return client.session ? 0 : 1;
}

static void
apple_client_shutdown(void *_client)
{
    SentryURLSessionClient *client = (__bridge SentryURLSessionClient *)_client;
    @synchronized(client) {
        client.shutdown = YES;
        [client.task cancel];
    }
}

static void
apple_client_free(void *_client)
{
    SentryURLSessionClient *client
        = (__bridge_transfer SentryURLSessionClient *)_client;
    // the session retains its delegate until invalidated
    [client.session invalidateAndCancel];
}

static int
apple_send_task(void *_client, sentry_prepared_http_request_t *req,
    sentry_http_response_t *resp)
{
    SentryURLSessionClient *client = (__bridge SentryURLSessionClient *)_client;
    @autoreleasepool {
        NSURL *url = [NSURL URLWithString:@(req->url)];
        if (!url) {
            SENTRY_WARNF("invalid request URL \"%s\"", req->url);
            return false;
        }
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
        request.HTTPMethod = @(req->method);
        for (size_t i = 0; i < req->headers_len; i++) {
            if (req->headers[i].key && req->headers[i].value) {
                [request setValue:@(req->headers[i].value)
                    forHTTPHeaderField:@(req->headers[i].key)];
            }
        }
        if (client.userAgent) {
            [request setValue:client.userAgent
                forHTTPHeaderField:@"User-Agent"];
        }
        if (client.proxyAuthorization && [url.scheme isEqualToString:@"http"]) {
            // NSURLSession never answers a 407 for plain HTTP requests, so
            // send the credentials up front like curl does
            [request setValue:client.proxyAuthorization
                forHTTPHeaderField:@"Proxy-Authorization"];
        }

        if (req->body_path) {
            FILE *file
                = sentry__path_open(req->body_path, "rb", req->body_offset);
            if (!file) {
                SENTRY_WARNF("failed to open request body file \"%s\" at "
                             "offset %zu",
                    sentry__path_filename(req->body_path), req->body_offset);
                return false;
            }
            request.HTTPBodyStream =
                [[SentryFileBodyStream alloc] initWithFile:file
                                                    length:req->body_len];
            // NSURLSession sends body streams chunked without a length
            [request setValue:@(req->body_len).stringValue
                forHTTPHeaderField:@"Content-Length"];
        } else if (req->body) {
            // the request outlives the synchronous send
            request.HTTPBody = [NSData dataWithBytesNoCopy:req->body
                                                    length:req->body_len
                                              freeWhenDone:NO];
        }

        __block bool ok = false;
        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        @synchronized(client) {
            if (client.shutdown) {
                return false;
            }
            client.task = [client.session
                dataTaskWithRequest:request
                  completionHandler:^(
                      NSData *data, NSURLResponse *response, NSError *error) {
                      (void)data;
                      if (error) {
                          SENTRY_WARNF("`NSURLSession` request failed: %s",
                              error.localizedDescription.UTF8String);
                      } else if ([response
                                     isKindOfClass:[NSHTTPURLResponse class]]) {
                          NSHTTPURLResponse *http
                              = (NSHTTPURLResponse *)response;
                          sentry_http_response_set_status_code(
                              resp, (int)http.statusCode);
                          [http.allHeaderFields
                              enumerateKeysAndObjectsUsingBlock:^(
                                  id key, id value, BOOL *stop) {
                                  (void)stop;
                                  if ([key isKindOfClass:[NSString class]] &&
                                      [value isKindOfClass:[NSString class]]) {
                                      sentry_http_response_set_header(resp,
                                          [key UTF8String], [value UTF8String]);
                                  }
                              }];
                          ok = true;
                      }
                      dispatch_semaphore_signal(done);
                  }];
            [client.task resume];
        }
        dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
        @synchronized(client) {
            client.task = nil;
        }
        return ok;
    }
}

sentry_transport_t *
sentry_transport_new_default(void)
{
    SENTRY_INFO("initializing apple transport");
    SentryURLSessionClient *client = [[SentryURLSessionClient alloc] init];
    if (!client) {
        return NULL;
    }

    void *state = (__bridge_retained void *)client;
    sentry_transport_t *transport
        = sentry__http_transport_new(state, apple_send_task);
    if (!transport) {
        apple_client_free(state);
        return NULL;
    }
    sentry__http_transport_set_free_client(transport, apple_client_free);
    sentry__http_transport_set_start_client(transport, apple_client_start);
    sentry__http_transport_set_shutdown_client(
        transport, apple_client_shutdown);
    return transport;
}
