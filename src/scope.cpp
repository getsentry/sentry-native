#include <mutex>

#include "modulefinder.hpp"
#include "options.hpp"
#include "symbolize.hpp"

#include "scope.hpp"

using namespace sentry;

static Scope g_scope;
static std::mutex scope_lock;

void Scope::with_scope(std::function<void(Scope &)> func) {
    std::lock_guard<std::mutex> _slck(scope_lock);
    func(g_scope);
}

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

    for (size_t i = 0, n = threads.length(); i < n; i++) {
        Value stacktrace = threads.get_by_index(i).get_by_key("stacktrace");
        if (stacktrace.type() == SENTRY_VALUE_TYPE_OBJECT) {
            rv.push_back(stacktrace);
        }
    }

    Value exc = event.get_by_key("exception");
    if (exc.type() == SENTRY_VALUE_TYPE_OBJECT) {
        Value exceptions = threads.get_by_key("values");
        if (exceptions.type() == SENTRY_VALUE_TYPE_OBJECT) {
            exc = exceptions;
        }
    }

    for (size_t i = 0, n = exc.length(); i < n; i++) {
        Value stacktrace = exc.get_by_index(i).get_by_key("stacktrace");
        if (stacktrace.type() == SENTRY_VALUE_TYPE_OBJECT) {
            rv.push_back(stacktrace);
        }
    }

    return rv;
}

static void postprocess_stacktrace(Value stacktrace) {
    Value frames = stacktrace.get_by_key("frames");
    if (frames.is_null() || frames.length() == 0) {
        return;
    }

    for (size_t i = 0, n = frames.length(); i < n; i++) {
        Value frame = frames.get_by_index(i);
        Value addr_value = frame.get_by_key("instruction_addr");
        if (addr_value.is_null()) {
            continue;
        }
        symbolizers::symbolize(
            (void *)addr_value.as_addr(),
            [&frame](const symbolizers::FrameInfo *frame_info) {
                if (frame.get_by_key("function").is_null() &&
                    frame_info->symbol) {
                    frame.set_by_key("function",
                                     Value::new_string(frame_info->symbol));
                }
                if (frame.get_by_key("filename").is_null() &&
                    frame_info->filename) {
                    frame.set_by_key("filename",
                                     Value::new_string(frame_info->filename));
                }
                if (frame.get_by_key("package").is_null() &&
                    frame_info->object_name) {
                    frame.set_by_key(
                        "package", Value::new_string(frame_info->object_name));
                }
                if (frame.get_by_key("symbol_addr").is_null() &&
                    frame_info->symbol_addr) {
                    frame.set_by_key(
                        "symbol_addr",
                        Value::new_addr((uint64_t)frame_info->symbol_addr));
                }
            });
    }
}

void Scope::apply_to_event(Value &event, ScopeMode mode) const {
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
    event.merge_key("contexts", contexts);

    if (fingerprint.type() == SENTRY_VALUE_TYPE_LIST &&
        fingerprint.length() > 0) {
        event.set_by_key("fingerprint", fingerprint);
    }

    // make sure to clone the breadcrumbs so that concurrent modifications
    // on the list after the scope lock was released do not cause a crash.
    if ((mode & SENTRY_SCOPE_BREADCRUMBS) && breadcrumbs.length() > 0) {
        event.merge_key("breadcrumbs", breadcrumbs.clone());
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
        sdk_info.freeze();
        shared_sdk_info = sdk_info;
    }

    if (mode & SENTRY_SCOPE_MODULES) {
        Value modules(modulefinders::get_module_list());
        if (!modules.is_null()) {
            Value debug_meta = Value::new_object();
            debug_meta.set_by_key("images", modules);
            event.set_by_key("debug_meta", debug_meta);
        }
    }

    if (mode & SENTRY_SCOPE_STACKTRACES) {
        std::vector<Value> stacktraces = find_stacktraces_in_event(event);
        for (auto iter = stacktraces.begin(); iter != stacktraces.end();
             ++iter) {
            postprocess_stacktrace(*iter);
        }
    }

    event.set_by_key("sdk", shared_sdk_info);
}
