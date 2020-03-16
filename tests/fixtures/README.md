The files here are created like this:

    $ gcc -shared -o with-buildid.so test.c
    $ gcc -Wl,--build-id=none -shared -o without-buildid.so test.c

For `with-buildid.so`, `sentry-cli difutil check` outputs the following:

    > debug_id: 4247301c-14f1-5421-53a8-a777f6cdb3a2 (x86_64)
    > code_id: 1c304742f114215453a8a777f6cdb3a2b8505e11 (x86_64)

For `without-buildid.so`:

    > debug_id: 29271919-a2ef-129d-9aac-be85a0948d9c (x86_64)

This is the value we want to replicate with our fallback hashing
