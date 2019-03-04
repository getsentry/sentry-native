// https://github.com/chmike/CxxUrl/blob/7e7af0ce97b1021eaaf687ced1f65a5bd3e7d548/url.hpp
#ifndef STRING_H
#define STRING_H

#include <string>
#include <sstream>

#ifdef ANDROID_PLATFORM
namespace std
{
template <typename T>
std::string to_string(const T &n)
{
    std::ostringstream stm;
    stm << n;
    return stm.str();
}
} // namespace std
#endif

#endif