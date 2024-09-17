/**
 * @file utils.hpp
 * @brief Utility functions for socket operations and other helper methods.
 *
 * This file contains utility functions that assist with socket operations and string validation tools.
 */

#ifndef BONJOUR_FOR_CPP_UTILS_HPP
#define BONJOUR_FOR_CPP_UTILS_HPP

#include <sys/select.h>

#include <cstring>
#include <string>

namespace impl
{
    /**
     * @brief Waits for activity on a given socket until a timeout occurs.
     *
     * Waits for activity (such as incoming data) on the specified socket for a duration defined by `timeout_secs`
     * and `timeout_usecs`. If the timeout period elapses without any activity on the socket, the function returns.
     *
     * @param socket The socket file descriptor to wait on.
     * @param timeout_secs The timeout duration in seconds.
     * @param timeout_usecs The additional timeout duration in microseconds.
     * @return Returns 0 if the timeout period elapses without any activity on the socket. Returns 1 if there is
     *         activity on the socket within the timeout period. Returns -1 if an error occurs.
     */
    
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
    
    /**
     * @brief Validates a given name string and returns a standardized version of the name.
     *
     * This function checks the validity of the input name string and corrects any issues it finds. 
     *
     * @param name A C-string representing the name to be validated.
     * @return A `std::string` containing the validated name.
     */
    
    std::string validate_name(const char *name)
    {
        return name;
    }
    
    /**
     * @brief Validates a given registration type (regtype) string and returns a standardized version of it.
     *
     * This function checks the validity of the input registration type and corrects any issues it finds. 
     *
     * @param regtype A C-string representing the registration type to be validated.
     * @return A `std::string` containing the validated registration type.
     */
    
    std::string validate_regtype(const char *regtype)
    {
        return regtype;
    }
    
    /**
     * @brief Validates a given domain string and returns a standardized version of it.
     *
     * This function checks the validity of the input domain string and corrects any issues it finds. 
     *
     * @param domain A C-string representing the domain to be validated.
     * @return A `std::string` containing the validated domain.
     */
    
    std::string validate_domain(const char *domain)
    {
        return (!domain || !strlen(domain)) ? "local." : domain;
    }
}

#endif /* BONJOUR_FOR_CPP_UTILS_HPP */
