
#ifndef BONJOUR_FOR_CPP_UTILS_HPP
#define BONJOUR_FOR_CPP_UTILS_HPP

#include <sys/select.h>

#include <cstring>
#include <string>

namespace impl
{
    int wait_on_socket(int socket, int timeout_secs, int timeout_usecs)
    {
        fd_set read, write, except;
        struct timeval timeout;

        // Timeout
        
        timeout.tv_sec = timeout_secs;
        timeout.tv_usec = timeout_usecs;
    
        // Sets

        FD_ZERO(&read);
        FD_ZERO(&write);
        FD_ZERO(&except);
            
        FD_SET(socket, &read);
            
        return select(socket + 1, &read, &write, &except, &timeout);
    }
    
    std::string validate_name(const char *name)
    {
        return name;
    }
    
    std::string validate_regtype(const char *regtype)
    {
        return regtype;
    }
    
    std::string validate_domain(const char *domain)
    {
        return (!domain || !strlen(domain)) ? "local." : domain;
    }
}

#endif /* BONJOUR_FOR_CPP_UTILS_HPP */
