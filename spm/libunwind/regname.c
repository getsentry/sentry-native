#if defined(__x86_64__)
#    include "../../../vendor/libunwind/src/x86_64/regname.c"
#elif defined(__aarch64__)
#    include "../../../vendor/libunwind/src/aarch64/regname.c"
#endif
