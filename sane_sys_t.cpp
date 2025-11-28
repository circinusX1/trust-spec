#include "sane_sys_t.h"

sane_sys_t::sane_sys_t(const LikeJs& cfg):cfg_(cfg)
{
}

bool sane_sys_t::create(std::fstream& iofile, int& line)
{
    return _common(iofile,true,line);
}

bool sane_sys_t::verify(std::fstream& iofile, int& line)
{
    return _common(iofile,false,line);
}
