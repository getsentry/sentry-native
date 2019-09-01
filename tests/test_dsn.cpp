#include <dsn.hpp>
#include <internal.hpp>
#include <string>
#include <vendor/catch.hpp>

TEST_CASE("simple https dsn parsing", "[dsn]") {
    sentry::Dsn dsn("https://public:private@host.com:444/42");
    REQUIRE(dsn.disabled() == false);
    REQUIRE(dsn.valid() == true);
    REQUIRE(dsn.is_secure() == true);
    REQUIRE(dsn.scheme() == std::string("https"));
    REQUIRE(dsn.public_key() == std::string("public"));
    REQUIRE(dsn.private_key() == std::string("private"));
    REQUIRE(dsn.host() == std::string("host.com"));
    REQUIRE(dsn.port() == 444);
    REQUIRE(dsn.path() == std::string(""));
    REQUIRE(dsn.project_id() == std::string("42"));
    REQUIRE(
        dsn.get_minidump_url() ==
        std::string("https://host.com:444/api/42/minidump/?sentry_key=public"));
    REQUIRE(dsn.get_store_url() ==
            std::string("https://host.com:444/api/42/store/"));
    REQUIRE(dsn.get_auth_header() ==
            std::string("Sentry sentry_key=public, sentry_version=7, "
                        "sentry_client=") +
                SENTRY_SDK_USER_AGENT);
}

TEST_CASE("simple http dsn parsing", "[dsn]") {
    sentry::Dsn dsn("http://public:private@host.com:8080/42");
    REQUIRE(dsn.disabled() == false);
    REQUIRE(dsn.valid() == true);
    REQUIRE(dsn.is_secure() == false);
    REQUIRE(dsn.scheme() == std::string("http"));
    REQUIRE(dsn.public_key() == std::string("public"));
    REQUIRE(dsn.private_key() == std::string("private"));
    REQUIRE(dsn.host() == std::string("host.com"));
    REQUIRE(dsn.port() == 8080);
    REQUIRE(dsn.path() == std::string(""));
    REQUIRE(dsn.project_id() == std::string("42"));
    REQUIRE(
        dsn.get_minidump_url() ==
        std::string("http://host.com:8080/api/42/minidump/?sentry_key=public"));
    REQUIRE(dsn.get_store_url() ==
            std::string("http://host.com:8080/api/42/store/"));
    REQUIRE(dsn.get_auth_header() ==
            std::string("Sentry sentry_key=public, sentry_version=7, "
                        "sentry_client=") +
                SENTRY_SDK_USER_AGENT);
}

TEST_CASE("dsn with path parsing", "[dsn]") {
    sentry::Dsn dsn("http://public:private@host.com:8080/path/here/42");
    REQUIRE(dsn.disabled() == false);
    REQUIRE(dsn.valid() == true);
    REQUIRE(dsn.is_secure() == false);
    REQUIRE(dsn.scheme() == std::string("http"));
    REQUIRE(dsn.public_key() == std::string("public"));
    REQUIRE(dsn.private_key() == std::string("private"));
    REQUIRE(dsn.host() == std::string("host.com"));
    REQUIRE(dsn.port() == 8080);
    REQUIRE(dsn.path() == std::string("path/here/"));
    REQUIRE(dsn.project_id() == std::string("42"));
    REQUIRE(dsn.get_minidump_url() ==
            std::string("http://host.com:8080/path/here/api/42/minidump/"
                        "?sentry_key=public"));
    REQUIRE(dsn.get_store_url() ==
            std::string("http://host.com:8080/path/here/api/42/store/"));
}

TEST_CASE("empty dsn parsing", "[dsn]") {
    sentry::Dsn dsn("");
    REQUIRE(dsn.disabled() == true);
    REQUIRE(dsn.valid() == true);
}
