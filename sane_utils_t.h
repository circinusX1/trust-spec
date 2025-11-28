#ifndef SANE_UTILS_T_H
#define SANE_UTILS_T_H

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <chrono>
#include <string>
#include <cstring>
#include <cstdint>

namespace  fs = std::filesystem;

inline bool is_number(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
};

bool match_wild(const std::string& filename, const std::string& wildcardPattern);

/**
 * @brief proc_exec
 * @param cmd
 * @return
 */
std::string proc_exec(const char* cmd);

inline size_t split_str(const std::string &str, char delim,
                        std::vector<std::string> &a)
{
    if (str.empty())
        return 0;

    std::string token;
    size_t prev = 0, pos = 0;
    size_t sl = str.length();
    a.clear();
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos)
            pos = sl;
        token = str.substr(prev, pos - prev);
        if (!token.empty())
            a.push_back(token);
        prev = pos + 1;
    } while (pos < sl && prev < sl);
    return a.size();
}

#endif // SANE_UTILS_T_H
