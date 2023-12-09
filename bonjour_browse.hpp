
#ifndef BONJOUR_BROWSE_HPP
#define BONJOUR_BROWSE_HPP

#include "bonjour_base.hpp"
#include "bonjour_named.hpp"

#include <list>

// An object for browsing bonjour services
// Makes a list of named services available, but does not resolve them

class bonjour_browse : public bonjour_base
{
public:

    static constexpr auto service = DNSServiceBrowse;
    
    using callback = DNSServiceBrowseReply;
    using callback_type = make_callback_type<bonjour_browse, 3, 1, 4, 5, 6>;

    friend callback_type;

    struct notify_type
    {
        notify_type() : m_stop(nullptr), m_add(nullptr), m_remove(nullptr) {}
        
        bonjour_notify<bonjour_browse>::stop_type m_stop;
        bonjour_notify<bonjour_browse>::state_type m_add;
        bonjour_notify<bonjour_browse>::state_type m_remove;
    };
    
    bonjour_browse(const char *regtype, const char *domain, notify_type notify = notify_type())
    : bonjour_base(regtype, domain)
    , m_notify(notify)
    {}
    
    bonjour_browse(bonjour_browse const& rhs) = delete;
    bonjour_browse(bonjour_browse const&& rhs) = delete;
    void operator = (bonjour_browse const& rhs) = delete;
    void operator = (bonjour_browse const&& rhs) = delete;
    
    bool start()
    {
        clear();
        return spawn(this, regtype(), domain());
    }
    
    void clear()
    {
        mutex_lock lock(m_mutex);
        m_services.clear();
    }
    
    void list_services(std::list<bonjour_named> &services)
    {
        mutex_lock lock(m_mutex);
        services = m_services;
    }
    
private:
    
    void reply(DNSServiceFlags flags, const char *name, const char *regtype, const char *domain)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;
        
        bonjour_named named(name, regtype, domain);
        
        auto it = named.find(m_services);
        
        if (flags & kDNSServiceFlagsAdd)
        {
            if (it == m_services.end())
                m_services.push_back(named);
                
            notify(m_notify.m_add, this, name, regtype, domain, complete);
        }
        else
        {
            if (it != m_services.end())
                m_services.erase(it);
            
            notify(m_notify.m_remove, this, name, regtype, domain, complete);
        }        
    }
    
    std::list<bonjour_named> m_services;
    
    notify_type m_notify;
};

#endif /* BONJOUR_BROWSE_HPP */
