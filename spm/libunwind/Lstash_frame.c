#if defined(__x86_64__)
#    include "../../../vendor/libunwind/src/x86_64/Lstash_frame.c"
#elif defined(__aarch64__)
#    include "../../../vendor/libunwind/src/aarch64/Lstash_frame.c"
#endif
