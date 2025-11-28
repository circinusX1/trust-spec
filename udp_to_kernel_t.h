#ifndef UDP_TO_KERNEL_T_H
#define UDP_TO_KERNEL_T_H
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#define PORT 15390
#define IP_ADDRESS "127.0.0.1"

#include <iostream> // For std::cout
#include <string>   // For std::string
#include <sstream>  // Essential for std::ostringstream

class udp_to_kernel_t
{
public:
    udp_to_kernel_t();
    ~udp_to_kernel_t();

    template <typename... Args>
    void ksend(Args&&... args)
    {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        oss.str();
        std::cerr << oss.str() << "\n";
        ::sendto(fd_socket_,oss.str().c_str(),oss.str().length(),0,(const struct sockaddr *) &sock_addr_, sizeof(sock_addr_));
    }

private:
    int fd_socket_;
    struct sockaddr_in sock_addr_;
};

extern udp_to_kernel_t* p_uper;
#define NOTY_KERN if(p_uper) p_uper->ksend

#endif // UDP_TO_KERNEL_T_H
