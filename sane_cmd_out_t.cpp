#include "sane_cmd_out_t.h"
#include "sane_files_t.h"
#include "paralel_crc_t.h"
#include "udp_to_kernel_t.h"

sane_cmd_out_t::sane_cmd_out_t(const LikeJs& cfg):sane_sys_t(cfg)
{
    const LikeJs::Node& pc = cfg["cmds"];
    command_t   cmd;

    for(size_t i=0;i<pc.count();i++)
    {
        cmd.command = pc.value(i);
        for(int j=0;j<pc[cmd.command.c_str()]["exclude"].count();j++)
        {
            cmd.excludes.push_back( pc[cmd.command.c_str()]["exclude"].value(j));
        }
        cmds_.push_back(cmd);
        cmd.excludes.clear();
    }
}

bool sane_cmd_out_t::_common(std::fstream& iofile, bool create, int& lineno)
{
    for(const auto& c : cmds_)
    {
        std::string line;

        std::string proc_exec;
        std::string proc_exec1  = ::proc_exec(c.command.c_str());
        std::stringstream sb;
        sb << proc_exec1;

        std::cout << c.command << std::endl;

        while (std::getline(sb, line))
        {
            for(const auto& re : c.excludes)
            {
                std::regex r(re);
                std::smatch m;
                std::regex_search(line, m, r);
                if(m.size()!=0)
                {
                    std::cout <<"line: " << lineno <<", cmd=" << line << " -> rejected \n";
                    continue;
                }
            }
            proc_exec.append(line);
        }

        if(!proc_exec.empty())
        {
            if(create)
            {
                iofile << lineno << ":P:" << c.command << ": " << proc_exec << std::endl;
            }
            else
            {
                std::stringstream out;
                std::string line;

                out << lineno << ":P:" << c.command << ": " << proc_exec << std::endl;
                std::getline(iofile, line);
                std::string curent = out.str();

                while(curent.back()<=' ')curent.pop_back();
                while(line.back()<=' ')line.pop_back();

                if(curent != line)
                {
                    NOTY_KERN("error: line=",lineno, ", comand=", line);
                    throw 2;
                }
            }
            ++lineno;
            NOTY_KERN("doing: line=" , lineno, ", cmd=", line);
        }
    }
}
