
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
    
    bonjour_named(const char *name, const char *regtype, const char *domain)
    : bonjour_base(regtype, domain)
    , m_name(impl::validate_name(name))
    {}
    
    const char *name() const
    {
        return m_name.c_str();
    }
    
    bool equal(const bonjour_named& b)
    {
        return equal(name(), b.name()) && equal(regtype(), b.regtype()) && equal(domain(), b.domain());
    }
    
    template <class T, std::enable_if_t<std::is_base_of<bonjour_named, T>::value, bool> = true>
    typename std::list<T>::iterator find(std::list<T>& list)
    {
        for (auto it = list.begin(); it != list.end(); it++)
            if (it->equal(*this))
                return it;
        
        return list.end();
    }
    
private:
    
    bool equal(const char *a, const char *b)
    {
        return !strcmp(a, b);
    }
    
    std::string m_name;
};

#endif /* BONJOUR_NAMED_HPP */
