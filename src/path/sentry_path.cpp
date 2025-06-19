extern "C" {
#include "sentry_path.h"
}

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

sentry_path_t *
sentry__path_unique(sentry_path_t *path)
{
    fs::path unique(path->path);
    if (!fs::exists(unique)) {
        return path;
    }

    // support common double-extensions like ".tar.gz"
    fs::path basename = unique.stem().stem();
    fs::path extension = unique.stem().extension();
    extension += unique.extension();

    // find the next available "filename-N.ext"
    size_t n = 1;
    constexpr size_t max_n = 4096; // arbitrary but reasonable limit
    do {
        fs::path filename = basename;
        filename += fs::path("-" + std::to_string(n));
        filename += extension;
        unique.replace_filename(filename);
    } while (fs::exists(unique) && ++n < max_n);

    if (n >= max_n || fs::exists(unique)) {
        return nullptr;
    }

    return sentry__path_new(unique.c_str());
}
