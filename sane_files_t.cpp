
#include "sane_files_t.h"
#include "paralel_crc_t.h"
#include "udp_to_kernel_t.h"

sane_files_t::sane_files_t(const LikeJs& cfg):sane_sys_t(cfg)
{
    const LikeJs::Node& pd = cfg["dirs"];
    director_t   ds;

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
            dirs_.push_back(ds);
            ds.includes.clear();
            ds.excludes.clear();

        }
    }
}

bool sane_files_t::_common(std::fstream& iofile, bool create, int& lineno)
{
    bool rv = true;
    size_t threads      = std::atol(cfg_["threads"].value().c_str());

    for(const auto& p : dirs_)
    {
        std::cout << "parsing: line=000000, folder=" << p.folder << std::endl;
        fs::path search_directory(p.folder);
        std::vector<fs::path> results = _find_files_in_dir(search_directory, p);

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
                NOTY_KERN("warning: line=",  lineno, ", file=", path.string(), ", what=", e.what());
            }
            if(create)
            {
                iofile << lineno << ":F:" << path.parent_path().string()<<"/"<< path.filename().string() <<
                    " s:" << fsz <<
                    " t:" << (uint64_t)ftime.time_since_epoch().count() <<
                    " c:" << std::hex << std::setw(8) << std::setfill('0') <<
                    crc << std::dec << std::endl;
                if(lineno %100==0)
                {
                    NOTY_KERN("creating: line=" , lineno, ", file=", path.filename().string());
                }
            }
            else
            {
                std::stringstream out;
                std::string line;

                std::getline(iofile, line);
                out << lineno << ":F:" << path.parent_path().string()<<"/"<< path.filename().string() <<
                    " s:" << fsz <<
                    " t:" << (uint64_t)ftime.time_since_epoch().count() <<
                    " c:" << std::hex << std::setw(8) << std::setfill('0') << crc << std::dec;
                std::string curent = out.str();
                if(curent != line)
                {
                    NOTY_KERN("error: line=", lineno,", cmd=", line, ", out=", curent);
                    throw 1;
                }
                if(lineno %100==0)
                {
                    NOTY_KERN("verify: line=",  lineno, ", file=", curent);
                }
            }
            ++lineno;
        }
    }
    return rv;
}

std::vector<fs::path> sane_files_t::_find_files_in_dir(const fs::path& root_path, const director_t& dr)
{
    std::vector<fs::path>   matching_files;
    std::error_code         ec;


    const auto filter=[](const std::string& name, const director_t& dr)->bool
    {
        if(!dr.includes.empty())
        {
            if (match_wild(name, dr.includes) == false)
            {
                return true;
            }
        }
        if(dr.excludes.size())
        {
            bool ignored = false;
            for(const auto& ign : dr.excludes)
            {
                if(match_wild(name, ign)==true)
                {
                    return true;
                }
            }
            return false;
        }
        return false;
    };

    try {
        if(dr.recusive)
        {
            for (const auto& entry : fs::recursive_directory_iterator(root_path,
                                                                      fs::directory_options::skip_permission_denied, ec))
            {
                const std::string name = entry.path().filename().string();
                if(filter(name,dr)==true)
                {
                    continue;
                }
                if (entry.is_regular_file())
                {
                    matching_files.push_back(entry.path());
                }
                if(dr.links)
                {
                    if(fs::exists(entry) && entry.is_symlink())
                    {
                        fs::path target_path = fs::read_symlink(entry);
                        std::string filename = target_path.filename().string();
                        matching_files.push_back(entry.path());
                    }
                }
            }
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(root_path,
                                                            fs::directory_options::skip_permission_denied, ec))
            {
                const std::string& name = entry.path().filename().string();
                if(filter(name,dr))
                {
                    continue;
                }

                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    matching_files.push_back(entry.path());
                }
                if(dr.links)
                {
                    if(fs::exists(entry) && entry.is_symlink())
                    {
                        fs::path target_path = fs::read_symlink(entry);
                        std::string filename = target_path.filename().string();
                        matching_files.push_back(entry.path());
                    }
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error: "  << e.what() << std::endl;
    }
    return matching_files;
}
