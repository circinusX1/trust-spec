
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unistd.h>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

class ini_file_t
{
public:

    ~ini_file_t()
    {
    }

    const std::string &key(const char *key) const
    {
        const auto &a = kv_.find(key);
        if (a != kv_.end())
        {
            return a->second;
        }
        return es_;
    }
    const std::string &operator[](const char *key) const
    {
        const auto &a = kv_.find(key);
        if (a != kv_.end())
            return a->second;
        return es_;
    }

    const std::map<std::string, std::string> &mapping() const { return kv_; }

    bool okay() const { return kv_.size() > 0; }

    static ini_file_t& get(const char* fcl, const char *app)
    {
        static ini_file_t r(fcl, app);
        return r;
    }


private:
    ini_file_t(const char* argv0, const char *f)
    {
        bool        found = false;
        std::string line;
        std::string file  = fs::path(argv0).parent_path().c_str();

        char* posibilities[256]= {
            "",
            "/etc/",
            nullptr
        };

        file += "/";
        file += f;
        file += ".conf";

        if(::access(file.c_str(),0)==0)
        {
            found = true;
        }
        else
        {
            for(char** p = posibilities; *p; p++)
            {
                file = *p; file += f;
                file += ".conf";
                if(::access(file.c_str(),0)==0){ found = true; break;}
            }
        }

        if(found)
        {
            std::ifstream fle;
            fle.open(file.c_str(), std::ifstream::in);
            if (fle.good())
            {
                cf_=file;
                while (getline(fle, line))
                {
                    if (line.empty())
                        continue;
                    if (line[0] == '#')
                        continue;
                    size_t pos = line.find_first_of("=");
                    if (pos != std::string::npos)
                    {
                        std::string key;
                        std::string value;
                        _ext(line, pos, key, value);
                        _tweak(value);
                        if (kv_.find(key) == kv_.end())
                        {
                            kv_[key] = value;
                        }
                        else
                        {
                            kv_[key].append("\n");
                            kv_[key].append(value);
                        }
                    }
                }
                fle.close();
            }
            else
            {
                std::cerr <<  "cannot open " << f;
            }
        }
        else
        {
            std::cerr <<"cannot find config file:" << f << ".conf";
            exit (2);
        }
    }

private:
    void _ext(const std::string& line, size_t pos, std::string& key, std::string& value)
    {
        size_t cmt = line.find_first_of('#');
        key  = line.substr(0, pos);
        value = line.substr(pos + 1, cmt > (pos+1) ? cmt-pos-1 : cmt);
        trim(key);
        trim(value);
    }

    inline void trim(std::string &s)
    {
        while (!s.empty() && s.at(0) <= ' ')
            s.erase(s.begin());
        while (!s.empty() && s.back() <= ' ')
            s.pop_back();
    }
    void _tweak(std::string& vvv)
    {
        if(vvv.find("${")!=std::string::npos)
        {
            const std::regex ereg{R"--(\$\{([^}]+)\})--"};
            std::smatch m;
            bool failed = true;
            while (std::regex_search(vvv, m, ereg))
            {
                if(m.size()==2)
                {
                    auto const from = m[0];
                    std::string evn = m[1].str();
                    const char* penv = std::getenv(evn.c_str());
                    if(penv==nullptr){
                        std::cerr << "env: '" << vvv  << "' not found. " <<
                            "at " << cf_ << "\n";
                        break;
                    }else{
                        std::string envv = penv;
                        vvv.replace(from.first, from.second, envv);
                        failed = false;
                    }
                }
            }
            if(failed)
            {
                // brute force
                size_t len = vvv.length();
                std::string tvar = vvv.substr(2,len-3);
                std::cerr <<"Trying to read env: '"<<tvar<<"'";
                const char* penv = std::getenv(tvar.c_str());
                if(penv==nullptr)
                {
                    std::cerr << "env: '" << tvar  << "' not found. " <<
                        "at " << cf_ << "\n";
                }
                else
                {
                    std::cerr << tvar << " READ using brute force ='" << penv <<"'";
                    std::string envv = penv;
                    vvv.replace(0, len, penv);
                }
            }
        }
    }

private:
    std::map<std::string, std::string> kv_;
    std::string es_;
    std::string cf_;
};
