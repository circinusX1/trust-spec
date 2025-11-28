// fast_crc_huge.cpp - Fast CRC32 for huge files (C++17, SIMD, multi-threaded)

#include <set>
#include "sane_utils_t.h"
#include "ini_file.h"
#include "paralel_crc_t.h"
#include "map_file_mem_t.h"
#include "udp_to_kernel_t.h"
#include "sane_files_t.h"
#include "sane_procs_t.h"
#include "sane_cmd_out_t.h"


int main(int argc, char* argv[])
{
    LikeJs  cfg;
    std::fstream  iofile;
    bool create = false;
    int lineno = 0;
    auto start = std::chrono::steady_clock::now();

    if(argc < 4)
    {
        std::cout << "C/V sumfile PUB/PKEY\n";
        return 0;
    }
    else
    {

        cfg.parse(argv[0]);
        NOTY_KERN(argv[0]," ",argv[1]," ",argv[2]," ",argv[3]);

        udp_to_kernel_t udper;
        sane_files_t    files(cfg);
        sane_cmd_out_t  cmds(cfg);
        sane_procs_t    procs(cfg);

        if(!::strcmp(argv[1],"V"))
        {
            std::string sig = argv[2]; sig += ".SIG";

            if(fs::is_regular_file(fs::path(sig.c_str())) && fs::is_regular_file(fs::path(argv[2])) &&
                fs::exists(argv[3]))
            {
                std::string cmd = "/usr/bin/openssl dgst -sha256 -verify ";
                cmd += argv[3]; cmd += " -signature "; cmd += sig;
                cmd += " ";  cmd += argv[2]; std::cout << cmd <<  "\n";
                int ret = ::system(cmd.c_str());
                if( WEXITSTATUS(ret) != 0 )
                {
                    NOTY_KERN("error: ", cmd);
                    return 1;
                }
            }
            try{
                std::fstream  iofile;

                iofile.open(argv[2],std::ios::in);
                files.verify(iofile,lineno);
                cmds.verify(iofile,lineno);
                procs.verify(iofile,lineno);
            }
            catch(int line)
            {
                NOTY_KERN("error: line:", line);
            }
            iofile.close();
        }
        else // CREATE
        {
            std::fstream  iofile;
            iofile.open(argv[2],std::ios::out);
            files.create(iofile,lineno);
            cmds.create(iofile,lineno);
            procs.create(iofile,lineno);
            iofile.close();

            if( fs::is_regular_file(fs::path(argv[2])) && fs::exists(argv[3]))
            {
                std::string cmd = "/usr/bin/openssl dgst -sha256 -sign ";
                std::string sig = argv[2]; sig += ".SIG";
                cmd += argv[3]; cmd += " -out "; cmd += sig;  cmd += " ";
                cmd += argv[2]; std::cout << cmd << "\n";
                NOTY_KERN(cmd);
                ::system(cmd.c_str());
                if(fs::is_regular_file(fs::path(sig.c_str())))
                {
                    NOTY_KERN(argv[2], " -> Signed ", argv[2], ".SIG");
                }
                else
                {
                    NOTY_KERN("error: ", argv[2], " -> Signed ", argv[2], ".SIG");
                }
            }
            else
            {
                NOTY_KERN("error: cannot find ", argv[2], " or ", argv[3]);
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "time:" << duration.count() << "ms \n";
    NOTY_KERN("time=", duration.count());
    return 0;
}

