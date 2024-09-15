
/**
 * @file bonjour_named.hpp
 * @brief Class for representing a named Bonjour service.
 *
 * This file defines the `bonjour_named` class, which represents a Bonjour service by its name, registration
 * type, and domain. It provides functionality for initializing and validating the service name but does not
 * resolve the service. To resolve a service, the `bonjour_service` class should be used. This class extends
 * the `bonjour_base` class, inheriting its core functionality for service management.
 */

#ifndef BONJOUR_NAMED_HPP
#define BONJOUR_NAMED_HPP

#include "bonjour_base.hpp"

#include <cstring>
#include <list>
#include <type_traits>

/**
 * @class bonjour_named
 * @brief Represents a named Bonjour service, inheriting from bonjour_base.
 *
 * The `bonjour_named` class is a specialized version of `bonjour_base`, which adds the functionality of
 * managing a service name. This class provides methods to compare the named service with either individual
 * 'bonjour_named' objects or lists of objects, and retrieve the stored name of the Bonjour service.
 *
 * Inherits common Bonjour service functionality from the `bonjour_base` class.
 */

class bonjour_named : public bonjour_base
{
public:
    
    /**
     * @brief Constructs a bonjour_named object with a specified name, registration type, and domain.
     *
     * This constructor validates and corrects the provided items to ensure that they are correctly formatted.
     *
     * @param name The name of the Bonjour service.
     * @param regtype The registration type of the Bonjour service (e.g., "_tcp").
     * @param domain The domain where the Bonjour service is registered (e.g., "local").
     */
    
    bonjour_named(const char *name, const char *regtype, const char *domain)
    : bonjour_base(regtype, domain)
    , m_name(impl::validate_name(name))
    {}
    
    /**
     * @brief Retrieves the name of the Bonjour service.
     *
     * This method returns the name of the Bonjour service as a C-string.
     *
     * @return The name of the Bonjour service as a C-string.
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
    
    /**
        * @brief Finds an element in the list that matches the current Bonjour service.
        *
        * This method searches through a list of Bonjour services (or derived classes) to find an element that is
        * equal to the current instance. It returns an iterator to the matching element if found, or the end
        * iterator if no match is found.
        *
        * @param list A reference to a list of Bonjour services (or derived classes) to search through.
        * @return An iterator to the matching element if found, or the end iterator if no match is found.
        */

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
     * @brief Compares two C-strings for equality.
     *
     * This method checks if two null-terminated C-strings are identical by comparing their
     * characters one by one. The comparison is case-sensitive.
     *
     * @param a A pointer to the first C-string to compare.
     * @param b A pointer to the second C-string to compare.
     * @return `true` if the strings are identical; `false` otherwise.
     */
    
    bool equal(const char *a, const char *b)
    {
        return !strcmp(a, b);
    }
    
    /** @brief Stores the validated name of the Bonjour service. */
    
    std::string m_name;
};

#endif /* BONJOUR_NAMED_HPP */
