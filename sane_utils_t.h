#ifndef SANE_UTILS_T_H
#define SANE_UTILS_T_H

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <chrono>
#include <string>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief _Socket_Fd
 */
extern int _Socket_Fd;

/**
 * @brief _Srv_Addr
 */
extern struct sockaddr_in _Srv_Addr;

/**
 * @brief The director_t class
 */
struct director_t{
    std::string folder;
    bool        recusive = false;
    bool        links = false;
    std::vector<std::string> excludes;
    std::string includes;
    int        crcszlen = 0;
};

/**
 * @brief The command_t class
 */
struct command_t
{
    std::string command;
    std::vector<std::string> excludes;
};
namespace  fs = std::filesystem;

/**
 * @brief wildcard_to_regex
 * @param wildcard
 * @return
 */
std::regex wildcard_to_regex(const std::string& wildcard);

/**
 * @brief find_files_in_dir
 * @param root_path
 * @param dr
 * @return
 */
std::vector<fs::path> find_files_in_dir(const fs::path& root_path, const director_t& dr);

/**
 * @brief proc_exec
 * @param cmd
 * @return
 */
std::string proc_exec(const char* cmd);

/**
 * @brief usend
 * @param args
 */
template <typename... Args>
void usend(Args&&... args)
{
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    oss.str();
    std::cerr << oss.str() << "\n";
    ::sendto(_Socket_Fd,oss.str().c_str(),oss.str().length(),0,(const struct sockaddr *) &_Srv_Addr, sizeof(_Srv_Addr));
}


#endif // SANE_UTILS_T_H
