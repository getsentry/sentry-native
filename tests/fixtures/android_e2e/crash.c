#include <jni.h>

__attribute__((noinline, optnone)) void
e2e_segfault(void)
{
    *(volatile int *)0 = 42;
}

__attribute__((noinline, optnone)) void
e2e_native_crash(void)
{
    e2e_segfault();
}

JNIEXPORT void JNICALL
Java_io_sentry_e2e_MainActivity_crash(JNIEnv *env, jclass cls)
{
    e2e_native_crash();
}
