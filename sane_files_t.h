#ifndef SANE_FILES_T_H
#define SANE_FILES_T_H

#include "sane_utils_t.h"
#include "sane_sys_t.h"

class sane_files_t : public sane_sys_t
{
    struct director_t{
        std::string folder;
        bool        recusive = false;
        bool        links = false;
        std::vector<std::string> excludes;
        std::string includes;
        int        crcszlen = 0;
    };
    std::vector<director_t>  dirs_;
    std::vector<fs::path> _find_files_in_dir(const fs::path& root_path, const director_t& dr);

protected:
    bool _common(std::fstream& file, bool create, int& line);

public:
    sane_files_t(const LikeJs& cfg);
    ~sane_files_t()=default;
};

#endif // SANE_FILES_T_H
