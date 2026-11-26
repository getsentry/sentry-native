#include <sentry.h>
#undef GetCurrentTime

#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Windows.Foundation.h>

#include <cstdio>
#include <cstring>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

static int g_argc = 0;
static char **g_argv = nullptr;

static bool
has_arg(const char *arg)
{
    for (int i = 1; i < g_argc; i++) {
        if (strcmp(g_argv[i], arg) == 0) {
            return true;
        }
    }
    return false;
}

static void
configure_initial_scope(sentry_scope_t *scope, void *)
{
    sentry_scope_set_tag(scope, "test.initial-tag", "initial-value");

    sentry_value_t context = sentry_value_new_object();
    sentry_value_set_by_key(context, "foo", sentry_value_new_string("bar"));
    sentry_scope_set_context(scope, "initial", context);
}

struct App : ApplicationT<App> {
    void
    OnLaunched(LaunchActivatedEventArgs const &)
    {
        Controls::TextBlock content;
        content.Loaded([](auto &&, auto &&) {
            if (has_arg("loaded-crash")) {
                sentry_crash();
            } else if (has_arg("loaded-exception")) {
                throw hresult_error(E_FAIL);
            }
        });
        m_window = Window();
        m_window.Content(content);
        m_window.DispatcherQueue().TryEnqueue([] {
            if (has_arg("dispatcher-crash")) {
                sentry_crash();
            } else if (has_arg("dispatcher-exception")) {
                throw hresult_error(E_FAIL);
            }
        });
        m_window.Activate();
    }

    Window m_window { nullptr };
};

int
main(int argc, char **argv)
{
    g_argc = argc;
    g_argv = argv;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_database_path(options, ".sentry-native");
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_environment(options, "development");
    sentry_options_set_release(options, "test-example-release");
    sentry_options_set_debug(options, 1);
    if (has_arg("initial-scope")) {
        sentry_options_set_initial_scope(
            options, configure_initial_scope, nullptr);
    }

    char e2e_test_id[37] = { 0 };
    if (has_arg("e2e-test")) {
        sentry_uuid_t test_uuid = sentry_uuid_new_v4();
        sentry_uuid_as_string(&test_uuid, e2e_test_id);
    }

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    if (e2e_test_id[0] != '\0') {
        sentry_set_tag("test.id", e2e_test_id);
        sentry_set_tag("test.suite", "e2e");
        printf("TEST_ID:%s\n", e2e_test_id);
        fflush(stdout);
    }

    sentry_set_transaction("test-transaction");
    sentry_set_extra("extra stuff", sentry_value_new_string("some value"));
    sentry_set_extra("…unicode key…",
        // https://xkcd.com/1813/ :-)
        sentry_value_new_string("őá…–🤮🚀¿ 한글 테스트"));
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");

    sentry_value_t context = sentry_value_new_object();
    sentry_value_set_by_key(
        context, "type", sentry_value_new_string("runtime"));
    sentry_value_set_by_key(
        context, "name", sentry_value_new_string("testing-runtime"));
    sentry_set_context("runtime", context);

    sentry_set_user(sentry_value_new_user(NULL, "some_name", NULL, NULL));

    init_apartment(apartment_type::single_threaded);
    Application::Start([](auto &&) { make<App>(); });

    sentry_close();
    return 0;
}
