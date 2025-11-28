#ifndef SANE_PROCS_T_H
#define SANE_PROCS_T_H

#include <set>
#include <string>
#include "sane_sys_t.h"

class sane_procs_t : public sane_sys_t
{
public:
    sane_procs_t(const LikeJs& cfg);
    ~sane_procs_t()=default;
    bool create(std::fstream& iofile, int& line);
    bool verify(std::fstream& iofile, int& line);

protected:
    bool _common(std::fstream& file, bool create, int& line);
    void _read(std::fstream& file);
    bool _compare(std::fstream& file);

protected:
    std::set<std::string>   sane_progs_;
    std::set<std::string>   curent_progs_;
};

#endif // SANE_PROCS_T_H
