
#include <iostream>
#include <string.h>
#include "udp_to_kernel_t.h"

udp_to_kernel_t* p_uper;

udp_to_kernel_t::udp_to_kernel_t()
{
    if ( (fd_socket_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        return;
    }

    ::memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDRESS, &sock_addr_.sin_addr) <= 0)
    {
        perror("invalid address/address not supported");
        close(fd_socket_);
        return;
    }
    p_uper = this;
}

udp_to_kernel_t::~udp_to_kernel_t()
{
    close(fd_socket_);
}

