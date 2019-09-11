#include "scope.hpp"
#include "options.hpp"

using namespace sentry;

static std::vector<Value> find_stacktraces_in_event(Value event) {
    std::vector<Value> rv;

    Value stacktrace = event.get_by_key("stacktrace");
    if (stacktrace.type() == SENTRY_VALUE_TYPE_OBJECT) {
        rv.push_back(stacktrace);
    }

    Value threads = event.get_by_key("threads");
    if (threads.type() == SENTRY_VALUE_TYPE_OBJECT) {
        threads = threads.get_by_key("values");
    }

    List *thread_list = threads.as_list();
    if (thread_list) {
        for (auto iter = thread_list->begin(); iter != thread_list->end();
             ++iter) {
            Value stacktrace = iter->get_by_key("stacktrace");
            if (stacktrace.type() == SENTRY_VALUE_TYPE_OBJECT) {
                rv.push_back(stacktrace);
            }
        }
    }

    Value exc = event.get_by_key("exception");
    if (exc.type() == SENTRY_VALUE_TYPE_OBJECT) {
        Value exceptions = threads.get_by_key("values");
        if (exceptions.type() == SENTRY_VALUE_TYPE_OBJECT) {
            exc = exceptions;
        }
    }

    List *exception_list = exc.as_list();
    if (exception_list) {
        for (auto iter = exception_list->begin(); iter != exception_list->end();
             ++iter) {
            Value stacktrace = iter->get_by_key("stacktrace");
            if (stacktrace.type() == SENTRY_VALUE_TYPE_OBJECT) {
                rv.push_back(stacktrace);
            }
        }
    }

    return rv;
}

static void postprocess_stacktrace(Value stacktrace) {
    List *frames = stacktrace.get_by_key("frames").as_list();
    if (!frames) {
        return;
    }

    for (auto iter = frames->begin(); iter != frames->end(); ++iter) {
        void *addr = iter->get_by_key("instruction_addr").as_addr();
    }
}

void Scope::apply_to_event(Value &event, bool with_breadcrumbs) const {
    const sentry_options_t *options = sentry_get_options();

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
    if (event.get_by_key("level").is_null()) {
        event.set_by_key("level", Value::new_level(level));
    }
    if (event.get_by_key("user").is_null()) {
        event.set_by_key("user", user);
    }
    if (!transaction.empty()) {
        event.set_by_key("transaction", Value::new_string(transaction.c_str()));
    }

    event.merge_key("tags", tags);
    event.merge_key("extra", extra);

    if (fingerprint.type() == SENTRY_VALUE_TYPE_LIST &&
        fingerprint.length() > 0) {
        event.set_by_key("fingerprint", fingerprint);
    }

    if (with_breadcrumbs && breadcrumbs.length() > 0) {
        event.merge_key("breadcrumbs", breadcrumbs);
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

    Value modules(sentry_get_module_list());
    if (!modules.is_null()) {
        Value debug_meta = Value::new_object();
        debug_meta.set_by_key("images", modules);
        event.set_by_key("debug_meta", debug_meta);
    }

    std::vector<Value> stacktraces = find_stacktraces_in_event(event);
    for (auto iter = stacktraces.begin(); iter != stacktraces.end(); ++iter) {
        postprocess_stacktrace(*iter);
    }

    event.set_by_key("sdk", shared_sdk_info);
}
