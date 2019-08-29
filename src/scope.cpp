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

void Scope::apply_to_event(Value &event, bool with_breadcrumbs) const {
    const sentry_options_t *options = sentry_get_options();

    // TODO: Merge instead of overwrite

    if (!options->release.empty()) {
        event.set_by_key("release",
                         Value::new_string(options->release.c_str()));
    }
    if (!options->dist.empty()) {
        event.set_by_key("dist", Value::new_string(options->dist.c_str()));
    }
    if (!options->environment.empty()) {
        event.set_by_key("environment",
                         Value::new_string(options->environment.c_str()));
    }
    event.set_by_key("level", Value::new_string(level_as_string(level)));
    event.set_by_key("user", user);
    if (!transaction.empty()) {
        event.set_by_key("transaction", Value::new_string(transaction.c_str()));
    }
    event.set_by_key("tags", tags);
    event.set_by_key("extra", extra);

    if (fingerprint.type() == SENTRY_VALUE_TYPE_LIST &&
        fingerprint.length() > 0) {
        event.set_by_key("fingerprint", fingerprint);
    }

    if (with_breadcrumbs && breadcrumbs.length() > 0) {
        event.set_by_key("breadcrumbs", breadcrumbs);
    }

    static Value shared_sdk_info;
    if (shared_sdk_info.is_null()) {
        Value sdk_info = Value::new_object();
        Value version = Value::new_string(SENTRY_SDK_VERSION);
        sdk_info.set_by_key("name", Value::new_string(SENTRY_SDK_NAME));
        sdk_info.set_by_key("version", version);
        Value package = Value::new_object();
        package.set_by_key("name",
                           Value::new_string("github:getsentry/sentry-native"));
        package.set_by_key("version", version);
        Value packages = Value::new_list();
        packages.append(package);
        sdk_info.set_by_key("packages", packages);
        shared_sdk_info = sdk_info;
    }

    if (options->module_finder) {
        Value debug_meta = Value::new_object();
        Value modules = options->module_finder->get_module_list();
        debug_meta.set_by_key("images", modules);
        event.set_by_key("debug_meta", debug_meta);
    }

    event.set_by_key("sdk", shared_sdk_info);
}
