#include "sane_procs_t.h"
#include "sane_utils_t.h"
#include "udp_to_kernel_t.h"


sane_procs_t::sane_procs_t(const LikeJs& cfg):sane_sys_t(cfg)
{
}

bool sane_procs_t::create(std::fstream& iofile, int& lineno)
{
    const LikeJs::Node& prg = cfg_["progs"];

    for(size_t i=0; i < prg.count(); i++)
    {
        iofile << lineno << ":process:" << prg.value(i) << std::endl;
        lineno++;
    }
    std::stringstream pout;
    std::string line;
    std::vector<std::string> parts;
    pout << ::proc_exec("ps ax");
    while (std::getline(pout, line))
    {
        split_str(line,' ',parts);
        if(parts.size() < 5)continue;
        if(parts[4][0]=='[')continue;
        std::string fullout;
        for(int i=4;i<parts.size();i++)
        {
            fullout+=parts[i];
            fullout+=" ";
        }
        iofile << lineno << ":process:" << fullout << std::endl;
        lineno++;
        NOTY_KERN("creating: line=" , lineno, ", process=", fullout);
    }
    return true;
}

bool sane_procs_t::verify(std::fstream& iofile, int& lineno)
{
    _read(iofile);
    return _compare(iofile);
}

void sane_procs_t::_read(std::fstream& iofile)
{
    const LikeJs::Node& prg = cfg_["progs"];

    for(size_t i=0; i < prg.count(); i++)
    {
        curent_progs_.insert(prg.value(i));
    }
    std::stringstream pout;
    std::string line;
    std::vector<std::string> parts;
    pout << ::proc_exec("ps ax");
    while (std::getline(pout, line))
    {
        split_str(line,' ',parts);
        if(parts.size() < 5)continue;
        if(parts[4][0]=='[')continue;
        std::string fullout;
        for(int i=4;i<parts.size();i++)
        {
            fullout+=parts[i];
            fullout+=" ";
        }
        curent_progs_.insert(fullout);
    }

    while(std::getline(iofile, line))
    {
        size_t index = line.find(":process:");
        if(index == std::string::npos)
        {
            break;
        }
        else
        {
            std::string process = line.substr(index+9);
            while(process.back()<=' ')process.pop_back();
            sane_progs_.insert(process);
        }
    };
    return;
}

bool sane_procs_t::_compare(std::fstream& iofile)
{
    std::string line;
    std::string worm;

    for(const auto& cp : curent_progs_)
    {
        for(const auto& sp : sane_progs_)
        {
            if( ::strstr(cp.c_str(),sp.c_str()) || ::strstr(sp.c_str(),cp.c_str()) )
            {
                worm.clear();
                break;
            }
            worm = cp;
        }
    }
    if(!worm.empty())
    {
        NOTY_KERN("error: worm=",worm);
        return false;
    }
    return true;
}


bool sane_procs_t::_common(std::fstream& iofile, bool create, int& lineno)
{
    return true;
}
