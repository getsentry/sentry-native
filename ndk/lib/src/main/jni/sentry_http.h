#ifndef SENTRY_ANDROID_HTTP_H_INCLUDED
#define SENTRY_ANDROID_HTTP_H_INCLUDED

#include <jni.h>
#include <sentry.h>

sentry_transport_t *sentry_android_http_transport_new(JNIEnv *env);

#endif
