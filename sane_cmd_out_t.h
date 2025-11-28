#ifndef SANE_CMD_OUT_T_H
#define SANE_CMD_OUT_T_H

#include "sane_sys_t.h"

class sane_cmd_out_t : public sane_sys_t
{
    struct command_t
    {
        std::string command;
        std::vector<std::string> excludes;
    };
    std::vector<command_t>  cmds_;

protected:
    bool _common(std::fstream& file, bool create, int& line);

public:
    sane_cmd_out_t(const LikeJs& cfg);
    ~sane_cmd_out_t()=default;
};

#endif // SANE_CMD_OUT_T_H
