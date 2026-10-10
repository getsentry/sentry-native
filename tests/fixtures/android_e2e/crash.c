#include <jni.h>

JNIEXPORT void JNICALL
Java_io_sentry_e2e_MainActivity_crash(JNIEnv *env, jclass cls)
{
    *(volatile int *)0 = 42;
}
