#ifndef SENTRY_BOOT_H_INCLUDED
#define SENTRY_BOOT_H_INCLUDED

// This header should always be included first.  Since we use clang-format
// it needs to be separated by a blank line to prevent automatic reordering.
//
// The purpose of this header is to configure some common system libraries
// before they are being used.  For instance this sets up some defines so
// that Windows.h is less polluting or that we get GNU extensions on linux.
//
// It also includes sentry.h since this is commonly used.

// we use some non portable extensions
#if defined(__linux__)
#    define _GNU_SOURCE
#endif

// make sure on windows we pull in a minimal Windows.h
#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#endif

#include <sentry.h>

#endif