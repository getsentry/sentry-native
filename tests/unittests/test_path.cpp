#include <internal.hpp>
#include <path.hpp>
#include <vendor/catch.hpp>

TEST_CASE("filename matches", "[path]") {
    bool is_windows = false;
#ifdef _WIN32
    is_windows = true;
#endif
    bool is_case_insensitive = is_windows;

    sentry::Path path("foo/bar/baz.txt");
    REQUIRE(path.filename_matches("baz.txt") == true);
    REQUIRE(path.filename_matches("baz.TXT") == is_case_insensitive);
    REQUIRE(path.filename_matches("baz.blah") == false);

    if (is_windows) {
        sentry::Path win_path("foo\\bar/baz.txt");
        REQUIRE(win_path.filename_matches("baz.txt") == true);
        win_path = "foo/bar\\blaz.txt";
        REQUIRE(win_path.filename_matches("blaz.txt") == true);
        REQUIRE(win_path.filename_matches("BLAZ.TXT") == true);
    }
}
