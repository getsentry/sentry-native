#include "sentry_http.h"

#include <android/log.h>
#include <jni.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

static JavaVM *g_vm;
static jclass g_client_class;
static jmethodID g_constructor;
static jmethodID g_configure;
static jmethodID g_send;
static jmethodID g_shutdown;
static jfieldID g_status_code;
static jfieldID g_headers;

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)reserved;
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass cls = (*env)->FindClass(env, "io/sentry/ndk/SentryHttpClient");
    if (!cls) {
        return JNI_ERR;
    }
    g_client_class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (!g_client_class) {
        return JNI_ERR;
    }
    jclass response
        = (*env)->FindClass(env, "io/sentry/ndk/SentryHttpClient$Response");
    if (!response
        || !(g_constructor
            = (*env)->GetMethodID(env, g_client_class, "<init>", "()V"))
        || !(g_configure = (*env)->GetMethodID(env, g_client_class, "configure",
                 "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"))
        || !(g_send = (*env)->GetMethodID(env, g_client_class, "send",
                 "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;"
                 "Ljava/nio/ByteBuffer;Ljava/lang/String;J)"
                 "Lio/sentry/ndk/SentryHttpClient$Response;"))
        || !(g_shutdown
            = (*env)->GetMethodID(env, g_client_class, "shutdown", "()V"))
        || !(g_status_code
            = (*env)->GetFieldID(env, response, "statusCode", "I"))
        || !(g_headers = (*env)->GetFieldID(
                 env, response, "headers", "[Ljava/lang/String;"))) {
        (*env)->DeleteGlobalRef(env, g_client_class);
        g_client_class = NULL;
        return JNI_ERR;
    }
    (*env)->DeleteLocalRef(env, response);
    g_vm = vm;
    return JNI_VERSION_1_6;
}

static JNIEnv *
get_env(bool *attached)
{
    JNIEnv *env = NULL;
    *attached = false;
    jint rv = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_6);
    if (rv == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
            return NULL;
        }
        *attached = true;
    } else if (rv != JNI_OK) {
        return NULL;
    }
    return env;
}

static bool
clear_exception(JNIEnv *env)
{
    if (!(*env)->ExceptionCheck(env)) {
        return false;
    }
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    __android_log_write(
        ANDROID_LOG_WARN, "sentry-native", "Android HTTP client failed");
    return true;
}

static void
release_env(JNIEnv *env, bool attached)
{
    clear_exception(env);
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

static sentry_http_client_t *
client_new(void *data)
{
    (void)data;
    bool attached;
    JNIEnv *env = get_env(&attached);
    if (!env) {
        return NULL;
    }
    jobject client = (*env)->NewObject(env, g_client_class, g_constructor);
    jobject ref = client ? (*env)->NewGlobalRef(env, client) : NULL;
    if (client) {
        (*env)->DeleteLocalRef(env, client);
    }
    release_env(env, attached);
    return ref;
}

static int
client_start(sentry_http_client_t *client, const sentry_options_t *options)
{
    bool attached;
    JNIEnv *env = get_env(&attached);
    if (!env) {
        return 1;
    }
    int rv = 1;
    if ((*env)->PushLocalFrame(env, 4) != JNI_OK) {
        release_env(env, attached);
        return rv;
    }
    const char *proxy = sentry_options_get_proxy(options);
    const char *ca_certs = sentry_options_get_ca_certs(options);
    const char *user_agent = sentry_options_get_user_agent(options);
    jstring jproxy = proxy ? (*env)->NewStringUTF(env, proxy) : NULL;
    if (proxy && !jproxy) {
        goto done;
    }
    jstring jca_certs = ca_certs ? (*env)->NewStringUTF(env, ca_certs) : NULL;
    if (ca_certs && !jca_certs) {
        goto done;
    }
    jstring juser_agent = (*env)->NewStringUTF(
        env, user_agent ? user_agent : SENTRY_SDK_USER_AGENT);
    if (!juser_agent) {
        goto done;
    }
    uint64_t timeout
        = sentry_options_get_transfer_timeout((sentry_options_t *)options);
    (*env)->CallVoidMethod(env, client, g_configure,
        (jlong)(timeout > INT64_MAX ? INT64_MAX : timeout), jproxy, jca_certs,
        juser_agent);
    rv = clear_exception(env) ? 1 : 0;

done:
    clear_exception(env);
    (*env)->PopLocalFrame(env, NULL);
    release_env(env, attached);
    return rv;
}

static int
client_send(sentry_http_client_t *client, sentry_http_request_t *req,
    sentry_http_response_t *resp)
{
    bool attached;
    JNIEnv *env = get_env(&attached);
    if (!env) {
        return 0;
    }
    int rv = 0;
    if ((*env)->PushLocalFrame(env, 16) != JNI_OK) {
        release_env(env, attached);
        return 0;
    }

    jstring method
        = (*env)->NewStringUTF(env, sentry_http_request_get_method(req));
    if (!method) {
        goto done;
    }
    jstring url = (*env)->NewStringUTF(env, sentry_http_request_get_url(req));
    if (!url) {
        goto done;
    }
    size_t count = sentry_http_request_get_header_count(req);
    if (count > INT_MAX / 2) {
        goto done;
    }
    jclass string_class = (*env)->FindClass(env, "java/lang/String");
    if (!string_class) {
        goto done;
    }
    jobjectArray headers
        = (*env)->NewObjectArray(env, (jsize)(count * 2), string_class, NULL);
    if (!headers) {
        goto done;
    }
    for (size_t i = 0; i < count; i++) {
        const char *key, *value;
        sentry_http_request_get_header(req, i, &key, &value);
        jstring jkey = (*env)->NewStringUTF(env, key);
        if (!jkey) {
            goto done;
        }
        jstring jvalue = (*env)->NewStringUTF(env, value);
        if (!jvalue) {
            goto done;
        }
        (*env)->SetObjectArrayElement(env, headers, (jsize)(i * 2), jkey);
        (*env)->SetObjectArrayElement(env, headers, (jsize)(i * 2 + 1), jvalue);
        (*env)->DeleteLocalRef(env, jkey);
        (*env)->DeleteLocalRef(env, jvalue);
        if ((*env)->ExceptionCheck(env)) {
            goto done;
        }
    }

    size_t body_len;
    const char *body = sentry_http_request_get_body(req, &body_len);
    jobject buffer = NULL;
    jstring path = NULL;
    if (body) {
        if (body_len > INT_MAX) {
            goto done;
        }
        buffer
            = (*env)->NewDirectByteBuffer(env, (void *)body, (jlong)body_len);
        if (!buffer) {
            goto done;
        }
    } else {
        const char *body_path
            = sentry_http_request_get_body_file_path(req, &body_len);
        if (body_path) {
            path = (*env)->NewStringUTF(env, body_path);
            if (!path) {
                goto done;
            }
        }
    }
    jobject response = (*env)->CallObjectMethod(env, client, g_send, method,
        url, headers, buffer, path, (jlong)body_len);
    if (clear_exception(env) || !response) {
        goto done;
    }
    sentry_http_response_set_status_code(
        resp, (*env)->GetIntField(env, response, g_status_code));
    jobjectArray response_headers
        = (*env)->GetObjectField(env, response, g_headers);
    jsize len = (*env)->GetArrayLength(env, response_headers);
    for (jsize i = 0; i < len; i += 2) {
        jstring key = (*env)->GetObjectArrayElement(env, response_headers, i);
        jstring value
            = (*env)->GetObjectArrayElement(env, response_headers, i + 1);
        const char *ckey = (*env)->GetStringUTFChars(env, key, NULL);
        if (!ckey) {
            goto done;
        }
        const char *cvalue = (*env)->GetStringUTFChars(env, value, NULL);
        if (cvalue) {
            sentry_http_response_set_header(resp, ckey, cvalue);
            (*env)->ReleaseStringUTFChars(env, value, cvalue);
        }
        (*env)->ReleaseStringUTFChars(env, key, ckey);
        (*env)->DeleteLocalRef(env, key);
        (*env)->DeleteLocalRef(env, value);
        if (!cvalue) {
            goto done;
        }
    }
    rv = 1;

done:
    clear_exception(env);
    (*env)->PopLocalFrame(env, NULL);
    release_env(env, attached);
    return rv;
}

static void
client_shutdown(sentry_http_client_t *client)
{
    bool attached;
    JNIEnv *env = get_env(&attached);
    if (env) {
        (*env)->CallVoidMethod(env, client, g_shutdown);
        release_env(env, attached);
    }
}

static void
client_free(sentry_http_client_t *client)
{
    client_shutdown(client);
    bool attached;
    JNIEnv *env = get_env(&attached);
    if (env) {
        (*env)->DeleteGlobalRef(env, client);
        release_env(env, attached);
    }
}

sentry_transport_t *
sentry_android_http_transport_new(void)
{
    sentry_transport_t *transport
        = sentry_http_transport_new(client_new, NULL, client_send, client_free);
    if (transport) {
        sentry_http_transport_set_client_start_func(transport, client_start);
        sentry_http_transport_set_client_shutdown_func(
            transport, client_shutdown);
    }
    return transport;
}
