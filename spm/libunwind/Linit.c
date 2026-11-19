#if defined(__x86_64__)
#    include "../../../vendor/libunwind/src/x86_64/Linit.c"
#elif defined(__aarch64__)
#    include "../../../vendor/libunwind/src/aarch64/Linit.c"
#endif
