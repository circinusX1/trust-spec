#ifndef SANE_SYS_T_H
#define SANE_SYS_T_H

#include <string>
#include "ini_file.h"

class sane_sys_t
{
public:
    sane_sys_t(const LikeJs& cfg);
    virtual ~sane_sys_t() = default;
    virtual bool create(std::fstream& iofile, int& line);
    virtual bool verify(std::fstream& iofile, int& line);

protected:
    virtual bool _common(std::fstream& iofile, bool create, int& lineno)=0;

protected:
    const LikeJs&   cfg_;
};

#endif // SANE_SYS_T_H
