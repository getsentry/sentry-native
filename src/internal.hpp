#ifndef SENTRY_INTERNAL_HPP_INCLUDED
#define SENTRY_INTERNAL_HPP_INCLUDED
#include <map>
#include <string>

#include "sentry.h"

// Char types
#ifdef _WIN32
typedef std::wstring xstring;
#else
typedef std::string xstring;
#endif

const sentry_options_t *sentry_get_options(void);

struct SentryInternalOptions {
    std::string minidump_url;
    /* The unique run identifier */
    std::string run_id;
    /* The full path to the current run */
    xstring run_path;
    std::map<std::string, xstring> attachments;
    sentry_options_t options;
};

static const int BREADCRUMB_MAX = 100;
static const int ATTACHMENTS_MAX = 100;
static const int ATTACHMENTS_KEY_LENGTH_MAX = 50;
static const int ATTACHMENTS_PATH_LENGTH_MAX = 260;
static const int MINIDUMP_URL_MAX_LENGTH = 150;

static const xchar_t *MINIDUMP_FILE_EXTENSION = XSTR(".dmp");
static const xchar_t *SENTRY_EVENT_FILE_NAME = XSTR("sentry-event.mp");
/* Refs are used to keep track of current breadcrumb file */
static const xchar_t *BREADCRUMB_FILE_1 = XSTR("sentry-breadcrumb1.mp");
static const xchar_t *BREADCRUMB_FILE_2 = XSTR("sentry-breadcrumb2.mp");
/* Names used when uploading to Sentry */
static const char *SENTRY_EVENT_FILE_ATTACHMENT_NAME = "__sentry-event";
static const char *SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME = "__sentry-breadcrumb1";
static const char *SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME = "__sentry-breadcrumb2";

static const char *SENTRY_SDK_VERSION = "0.0.1";
#endif
