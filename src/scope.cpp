#include "scope.hpp"
#include "options.hpp"

using namespace sentry;

static const char *level_as_string(sentry_level_t level) {
    switch (level) {
        case SENTRY_LEVEL_DEBUG:
            return "debug";
        case SENTRY_LEVEL_WARNING:
            return "warning";
        case SENTRY_LEVEL_ERROR:
            return "error";
        case SENTRY_LEVEL_FATAL:
            return "fatal";
        case SENTRY_LEVEL_INFO:
        default:
            return "info";
    }
}

void Scope::applyToEvent(Value &event, bool withBreadcrumbs) const {
    const sentry_options_t *options = sentry_get_options();

    // TODO: Merge instead of overwrite

    if (!options->release.empty()) {
        event.setKey("release", Value::newString(options->release.c_str()));
    }
    if (!options->dist.empty()) {
        event.setKey("dist", Value::newString(options->dist.c_str()));
    }
    if (!options->environment.empty()) {
        event.setKey("environment",
                     Value::newString(options->environment.c_str()));
    }
    event.setKey("level", Value::newString(level_as_string(level)));
    event.setKey("user", user);
    if (!transaction.empty()) {
        event.setKey("transaction", Value::newString(transaction.c_str()));
    }
    event.setKey("tags", tags);
    event.setKey("extra", extra);

    if (fingerprint.type() == SENTRY_VALUE_TYPE_LIST &&
        fingerprint.length() > 0) {
        event.setKey("fingerprint", fingerprint);
    }

    if (withBreadcrumbs && breadcrumbs.length() > 0) {
        event.setKey("breadcrumbs", breadcrumbs);
    }

    static Value sdk_info;
    if (sdk_info.isNull()) {
        Value version = Value::newString(SENTRY_SDK_VERSION);
        sdk_info = Value::newObject();
        sdk_info.setKey("name", Value::newString(SENTRY_SDK_NAME));
        sdk_info.setKey("version", version);
        Value package = Value::newObject();
        package.setKey("name", Value::newString("github:getsentry/sentrypad"));
        package.setKey("version", version);
        Value packages = Value::newList();
        packages.append(package);
        sdk_info.setKey("packages", packages);
    }

    event.setKey("sdk", sdk_info);
}
