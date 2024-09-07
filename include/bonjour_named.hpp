
/**
 * @file bonjour_named.hpp
 * @brief Represents a named Bonjour service.
 *
 * This file defines the `bonjour_named` class, which represents a Bonjour service
 * by its name, registration type, and domain. It provides functionality for
 * initializing and validating the service name but does not resolve the service.
 * To resolve a service, the `bonjour_service` class should be used. This class
 * extends the `bonjour_base` class, inheriting its core functionality for service
 * management.
 */

#ifndef BONJOUR_NAMED_HPP
#define BONJOUR_NAMED_HPP

#include "bonjour_base.hpp"

#include <cstring>
#include <list>
#include <type_traits>

// A representation of a named bonjour service
// Note that to resolve the named service you should use bonjour_service

class bonjour_named : public bonjour_base
{
public:
    
    /**
     * @brief Constructs a bonjour_named object with a specified name, registration type, and domain.
     *
     * This constructor initializes the bonjour_named object by calling the base class constructor
     * `bonjour_base` with the provided `regtype` and `domain`. It also validates and assigns the
     * provided `name` using the `impl::validate_name` method.
     *
     * @param name The name of the Bonjour service. This is validated using `impl::validate_name`.
     * @param regtype The registration type of the Bonjour service (e.g., "_http._tcp").
     * @param domain The domain where the Bonjour service is registered (e.g., "local").
     */
    
    bonjour_named(const char *name, const char *regtype, const char *domain)
    : bonjour_base(regtype, domain)
    , m_name(impl::validate_name(name))
    {}
    
    /**
     * @brief Retrieves the name of the Bonjour service.
     *
     * This method returns the name of the Bonjour service as a C-style string.
     *
     * @return A pointer to a constant character array representing the name of the Bonjour service.
     */
    
    const char *name() const
    {
        return m_name.c_str();
    }
    
    /**
     * @brief Compares this Bonjour service with another to check for equality.
     *
     * This method compares the current Bonjour service instance with another `bonjour_named` instance.
     * It checks if the name, registration type, and domain of both services are equal.
     *
     * @param b A reference to another `bonjour_named` object to compare against.
     * @return `true` if the name, registration type, and domain of both services are identical;
     *         `false` otherwise.
     */
    
    bool equal(const bonjour_named& b)
    {
        return equal(name(), b.name()) && equal(regtype(), b.regtype()) && equal(domain(), b.domain());
    }
    
    template <class T, std::enable_if_t<std::is_base_of<bonjour_named, T>::value, bool> = true>
    typename std::list<T>::iterator find(std::list<T>& list) const
    {
        for (auto it = list.begin(); it != list.end(); it++)
            if (it->equal(*this))
                return it;
        
        return list.end();
    }
    
private:
    
    /**
     * @brief Compares two C-style strings for equality.
     *
     * This method checks if two null-terminated C-style strings are identical by comparing their
     * characters one by one. The comparison is case-sensitive.
     *
     * @param a A pointer to the first C-style string to compare.
     * @param b A pointer to the second C-style string to compare.
     * @return `true` if the strings are identical; `false` otherwise.
     */
    
    bool equal(const char *a, const char *b)
    {
        return !strcmp(a, b);
    }
    
    std::string m_name;
};

#endif /* BONJOUR_NAMED_HPP */
