#include "sane_utils_t.h"


static std::string wild2reg(const std::string& wildcard, bool caseSensitive = false)
{
    if (wildcard.empty())
        return "^$";  // match nothing

    std::stringstream result;

    // If multiple patterns separated by '|'
    std::stringstream ss(wildcard);
    std::string pattern;
    bool first = true;

    if (wildcard.find('|') != std::string::npos) {
        result << "(?:";  // non-capturing group
        while (std::getline(ss, pattern, '|')) {
            if (!first) result << "|";
            first = false;

            result << "^";
            for (char c : pattern) {
                switch (c) {
                case '*': result << ".*"; break;
                case '?': result << ".";  break;
                case '.': result << "\\."; break;
                case '\\': result << "\\\\"; break;
                case '^': case '$': case '[': case ']':
                case '(': case ')': case '{': case '}':
                case '+': case '|':
                    result << '\\' << c;
                    break;
                default:
                    result << c;
                    break;
                }
            }
            result << "$";
        }
        result << ")";
    } else {
        // Single pattern
        result << "^";
        for (char c : wildcard) {
            switch (c) {
            case '*': result << ".*"; break;
            case '?': result << ".";  break;
            case '.': result << "\\."; break;
            case '\\': result << "\\\\"; break;
            case '^': case '$': case '[': case ']':
            case '(': case ')': case '{': case '}':
            case '+': case '|':
                result << '\\' << c;
                break;
            default:
                result << c;
                break;
            }
        }
        result << "$";
    }

    // Add case-insensitive flag if needed
    if (!caseSensitive) {
        result << "(?i)";  // This only works with std::regex in some implementations
        // Alternative: handle case-insensitivity at match time
    }

    return result.str();
}

bool match_wild(const std::string& filename, const std::string& wildcardPattern)
{
    bool rv = false;
    std::string regexStr = wild2reg(wildcardPattern, true);  // case-insensitive = false or true as needed
    try {
        std::regex re(regexStr, std::regex::icase);  // std::regex::icase makes it case-insensitive reliably
        rv =  std::regex_match(filename, re);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << " - pattern: " << regexStr << std::endl;
    }
    return rv;
}


std::string proc_exec(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (pipe)
    {
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            std::cout << "Exception " << cmd;
            throw;
        }
    }
    pclose(pipe);
    return result;
}

