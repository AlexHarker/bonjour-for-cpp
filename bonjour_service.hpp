
#ifndef BONJOUR_SERVICE_HPP
#define BONJOUR_SERVICE_HPP

#include "bonjour_named.hpp"

// An object for resolving a named bonjour service
// This can be constructed from separate names, or a bonjour_named object

class bonjour_service : public bonjour_named
{
public:
    
    static constexpr auto service = DNSServiceResolve;
    
    using callback = DNSServiceResolveReply;
    using callback_type = make_callback_type<bonjour_service, 3, 1, 4, 5, 6>;
    
    friend callback_type;
    
    struct notify_type
    {
        notify_type() : m_stop(nullptr), m_resolve(nullptr) {}

        bonjour_notify<bonjour_service>::stop_type m_stop = nullptr;
        bonjour_notify<bonjour_service>::resolve_type m_resolve = nullptr;
    };
    
    bonjour_service(bonjour_named named, notify_type notify = notify_type())
    : bonjour_named(named)
    , m_port(0)
    , m_notify(notify)
    {
        if (strlen(name()))
            resolve();
    }
    
    bonjour_service(const char *name, const char *regtype, const char *domain, notify_type notify = notify_type())
    : bonjour_service(bonjour_named(name, regtype, domain), notify)
    {}
    
    bonjour_service(bonjour_service const& rhs)
    : bonjour_named("", "", "")
    {
        *this = rhs;
    }
    
    void operator = (bonjour_service const& rhs)
    {
        mutex_lock lock1(m_mutex);
        mutex_lock lock2(rhs.m_mutex);
            
        stop();
        
        static_cast<bonjour_named&>(*this) = static_cast<const bonjour_named&>(rhs);
        
        m_fullname = rhs.m_fullname;
        m_host = rhs.m_host;
        m_port = rhs.m_port;
        m_notify = rhs.m_notify;
    }
    
    bool resolve()
    {
        return spawn(this, name(), regtype(), domain());
    }
    
    std::string fullname() const
    {
        mutex_lock lock(m_mutex);
        std::string str(m_fullname);
        return str;
    }
    
    std::string host() const
    {
        mutex_lock lock(m_mutex);
        std::string str(m_host);
        return str;
    }
    
    uint16_t port() const
    {
        mutex_lock lock(m_mutex);
        uint16_t port = m_port;
        return port;
    }
    
private:
    
    void reply(DNSServiceFlags flags, const char *fullname, const char *host, uint16_t port)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;

        m_fullname = fullname;
        m_host = host;
        m_port = port;
                
        stop();
        
        notify(m_notify.m_resolve, this, fullname, host, port, complete);
    }
            
    std::string m_fullname;
    std::string m_host;
    uint16_t m_port;
    
    notify_type m_notify;
};

#endif /* BONJOUR_SERVICE_HPP */
