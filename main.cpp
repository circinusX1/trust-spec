// fast_crc_huge.cpp - Fast CRC32 for huge files (C++17, SIMD, multi-threaded)


#include "sane_utils_t.h"
#include "ini_file.h"
#include "paralel_crc_t.h"
#include "map_file_mem_t.h"

#define PORT 15390
#define IP_ADDRESS "127.0.0.1"

/**
 * @brief _Socket_Fd
 */
int _Socket_Fd;
struct sockaddr_in _Srv_Addr;

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[])
{
    LikeJs  cfg;
    std::vector<director_t>  dirs;
    std::vector<command_t>  cmds;
    cfg.parse(argv[0]);

    size_t threads      = std::atol(cfg["threads"].value().c_str());
    const LikeJs::Node& pd = cfg["dirs"];
    const LikeJs::Node& pc = cfg["cmds"];
    std::string v ;
    director_t   ds;
    command_t   cmd;

    for(size_t i=0;i<pc.count();i++)
    {
        cmd.command = pc.value(i);
        for(int j=0;j<pc[cmd.command.c_str()]["ignore"].count();j++)
        {
            cmd.excludes.push_back( pc[cmd.command.c_str()]["ignore"].value(j));
        }
        cmds.push_back(cmd);
        cmd.excludes.clear();
    }

    for(size_t i=0;i<pd.count();i++)
    {
        ds.folder = pd.value(i);
        if(!ds.folder.empty())
        {
            ds.recusive = pd[ds.folder.c_str()]["recursive"].value() == "true";
            ds.includes = pd[ds.folder.c_str()]["include"].value();

            for(int j=0;j<pd[ds.folder.c_str()]["exclude"].count();j++)
            {
                ds.excludes.push_back( pd[ds.folder.c_str()]["exclude"].value(j));
            }
            ds.crcszlen = 0;
            for(int j=0;j<pd[ds.folder.c_str()]["what"].count();j++)
            {
                if(pd[ds.folder.c_str()]["what"].value(j)=="size")
                {
                    ds.crcszlen |= 1;
                }
                else if(pd[ds.folder.c_str()]["what"].value(j)=="time")
                {
                    ds.crcszlen |= 2;
                }
                else if(pd[ds.folder.c_str()]["what"].value(j)=="crc")
                {
                    ds.crcszlen |= 4;
                }
            }
            ds.links = pd[ds.folder.c_str()]["symlinks"].value() == "true";
            dirs.push_back(ds);

            ds.includes.clear();
            ds.excludes.clear();

        }
    }
    std::fstream  iofile;
    std::string    pkey;
    bool create = false;
    int rv = 0;

    if(argc < 4)     // create
    {
        std::cout << "create/verify sumfile <pkey/PKEY>\n";
        return 0;
    }

    if ( (_Socket_Fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        return 1;
    }

    memset(&_Srv_Addr, 0, sizeof(_Srv_Addr));
    _Srv_Addr.sin_family = AF_INET;
    _Srv_Addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDRESS, &_Srv_Addr.sin_addr) <= 0)
    {
        perror("invalid address/address not supported");
        close(_Socket_Fd);
        return 1;
    }
    ::usend(argv[0]," ",argv[1]," ",argv[2]," ",argv[3]);

    // $0 CREATE HASH PK
    if(!::strcmp(argv[1],"CREATE"))
    {
        iofile.open(argv[2],std::ios::out);
        pkey = argv[3];
        create = true;
    }

    else if(!::strcmp(argv[1],"VERIFY"))
    {
        // $0 VERIFY HASH PK
        std::string sig = argv[2]; sig += ".SIG";

        pkey = argv[3];
        if(fs::is_regular_file(fs::path(sig.c_str())) && fs::is_regular_file(fs::path(argv[2])) &&
            fs::exists(pkey.c_str()))
        {
            std::string cmd = "/usr/bin/openssl dgst -sha256 -verify ";
            cmd += pkey;
            cmd += " -signature ";
            cmd += sig;
            cmd += " ";
            cmd += argv[2];
            std::cout << cmd << "\n";
            int ret = ::system(cmd.c_str());
            if( WEXITSTATUS(ret) != 0 )
            {
                ::usend("error: ", cmd);
                return 1;
            }
        }
        iofile.open(argv[2],std::ios::in);
    }
    int lineno = 0;
    auto start = std::chrono::steady_clock::now();
    if(iofile.is_open())
    {
        try{

            for(const auto& p : dirs)
            {
                std::cout << p.folder << std::endl;
                fs::path search_directory(p.folder);
                std::vector<fs::path> results = find_files_in_dir(search_directory, p);

                for(const auto& path : results)
                {

                    size_t   fsz;
                    uint32_t crc = 2;
                    fs::file_time_type ftime = fs::last_write_time(path);
                    try
                    {
                        crc = crc_file_parallel(path, threads, fsz, p.crcszlen);
                    }
                    catch (const std::exception& e)
                    {
                        ::usend("warning: line=",  lineno, ", file=", path.string(), ", what=", e.what());
                    }
                    if(create)
                    {
                        iofile << lineno << ":" << path.parent_path().string()<<"/"<< path.filename().string() <<
                            " s:" << fsz <<
                            " t:" << (uint64_t)ftime.time_since_epoch().count() <<
                            " c:" << std::hex << std::setw(8) << std::setfill('0') <<
                            crc << std::dec << std::endl;
                        if(lineno %100==0)
                        {
                            ::usend("creating: line=" , lineno, ", file=", path.filename().string());
                        }
                    }
                    else
                    {
                        std::stringstream out;
                        std::string line;

                        std::getline(iofile, line);
                        out << lineno << ":" << path.parent_path().string()<<"/"<< path.filename().string() <<
                            " s:" << fsz <<
                            " t:" << (uint64_t)ftime.time_since_epoch().count() <<
                            " c:" << std::hex << std::setw(8) << std::setfill('0') << crc << std::dec;
                        std::string curent = out.str();
                        if(curent != line)
                        {
                            ::usend("error: line=", lineno,", cmd=", line, ", out=", curent);
                            throw 1;
                        }
                        if(lineno %100==0)
                        {
                            ::usend("verify: line=",  lineno, ", file=", curent);
                        }
                    }
                    ++lineno;

                }
            }
            --lineno;
            for(const auto& c : cmds)
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
                        iofile << lineno << ": " << cmd.command << ": " << proc_exec << std::endl;
                    }
                    else
                    {
                        std::stringstream out;
                        std::string line;

                        out << lineno << ": " << cmd.command << ": " << proc_exec << std::endl;
                        std::getline(iofile, line);
                        std::string curent = out.str();

                        while(curent.back()<=' ')curent.pop_back();
                        while(line.back()<=' ')line.pop_back();

                        if(curent != line)
                        {
                            ::usend("error: line=",lineno, ", comand=", line);
                            throw 2;
                        }
                    }
                }
                ++lineno;
            }
        }
        catch(int i)
        {
            rv = i;
            ::usend("error: exception=",i);
        }
    }
    else
    {
        ::usend("error: file=",argv[2]," not found");
    }
    iofile.close();

    if(create)
    {
        if( fs::is_regular_file(fs::path(argv[2])) && fs::exists(pkey.c_str()))
        {
            std::string cmd = "/usr/bin/openssl dgst -sha256 -sign ";
            std::string sig = argv[2]; sig += ".SIG";
            cmd += pkey;
            cmd += " -out ";
            cmd += sig;
            cmd += " ";
            cmd += argv[2];
            std::cout << cmd << "\n";
            usend(cmd);
            ::system(cmd.c_str());
            if(fs::is_regular_file(fs::path(sig.c_str())))
            {
                ::usend(argv[2], " -> Signed ", argv[2], ".SIG");
            }
            else
            {
                ::usend("error: ", argv[2], " -> Signed ", argv[2], ".SIG");
            }
        }
        else
        {
            ::usend("error: cannot find ", argv[2], " or ", pkey.c_str());
        }
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "time:" << duration.count() << "ms \n";
    ::usend("exit=",rv,", time=", duration.count());
    close(_Socket_Fd);
    return rv;
}

