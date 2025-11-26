#include "sane_utils_t.h"


std::string wildcardToRegex(const std::string& wildcard, bool caseSensitive = false)
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

bool matchesWildcard(const std::string& filename, const std::string& wildcardPattern)
{
    std::string regexStr = wildcardToRegex(wildcardPattern, true);  // case-insensitive = false or true as needed
    try {
        std::regex re(regexStr, std::regex::icase);  // std::regex::icase makes it case-insensitive reliably
        return std::regex_match(filename, re);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << " - pattern: " << regexStr << std::endl;
        return false;
    }
}

std::vector<fs::path> find_files_in_dir(const fs::path& root_path, const director_t& dr)
{
    std::vector<fs::path>   matching_files;
    std::error_code         ec;


    const auto filter=[](const std::string& name, const director_t& dr)->bool
    {
        if(dr.includes=="~error~")
        {
            return false;
        }
        if (matchesWildcard(name, dr.includes) == false)
        {
            std::cout << "reg exclude " << name << std::endl;
            return true;
        }
        if(dr.excludes.size())
        {
            bool ignored = false;
            for(const auto& ign : dr.excludes)
            {
                if(matchesWildcard(name,ign)==true)
                {
                    std::cout << "ign exclude " << name << std::endl;
                    return true;
                }
            }
            return false;
        }
        return false;
    };

    try {
        if(dr.recusive)
        {
            for (const auto& entry : fs::recursive_directory_iterator(root_path,
                                                                      fs::directory_options::skip_permission_denied, ec))
            {
                const std::string name = entry.path().filename().string();
                if(filter(name,dr)==true)
                {
                    continue;
                }
                if (entry.is_regular_file())
                {
                    matching_files.push_back(entry.path());
                }
                if(dr.links)
                {
                    if(fs::exists(entry) && entry.is_symlink())
                    {
                        fs::path target_path = fs::read_symlink(entry);
                        std::string filename = target_path.filename().string();
                         matching_files.push_back(entry.path());
                    }
                }
            }
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(root_path,
                                                            fs::directory_options::skip_permission_denied, ec))
            {
                const std::string& name = entry.path().filename().string();
                if(filter(name,dr))
                {
                    continue;
                }

                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    matching_files.push_back(entry.path());
                }
                if(dr.links)
                {
                    if(fs::exists(entry) && entry.is_symlink())
                    {
                        fs::path target_path = fs::read_symlink(entry);
                        std::string filename = target_path.filename().string();
                        matching_files.push_back(entry.path());
                    }
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error: "  << e.what() << std::endl;
    }
    return matching_files;
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

