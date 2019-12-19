#include <internal.hpp>
#include <io.hpp>
#include <path.hpp>
#include <vendor/catch.hpp>

#ifdef __ANDROID__
#define PREFIX "/data/local/tmp/"
#else
#define PREFIX ""
#endif

TEST_CASE("basic file reading and writing", "[io]") {
    sentry::Path path(PREFIX "testfile.txt");
    sentry::FileIoWriter w;
    REQUIRE(w.open(path) == true);
    for (int i = 0; i < 2048; i++) {
        w.write_int32(i % 10);
    }

    sentry::FileIoReader r;
    REQUIRE(r.open(path) == true);

    for (int i = 0; i < 2048; i++) {
        REQUIRE(r.read_char() - '0' == i % 10);
    }

    path.remove();
}
