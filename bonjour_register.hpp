
#ifndef BONJOUR_REGISTER_HPP
#define BONJOUR_REGISTER_HPP

#include "bonjour_named.hpp"

// An object for registering a named bonjour service

class bonjour_register : public bonjour_named
{
public:

    static constexpr auto service = DNSServiceRegister;
    
    using callback = DNSServiceRegisterReply;
    using callback_type = make_callback_type<bonjour_register, 2, 1, 3, 4, 5>;

    friend callback_type;
    
    struct notify_type
    {
        notify_type() : m_stop(nullptr), m_add(nullptr), m_remove(nullptr) {}

        bonjour_notify<bonjour_register>::stop_type m_stop = nullptr;
        bonjour_notify<bonjour_register>::state_type m_add = nullptr;
        bonjour_notify<bonjour_register>::state_type m_remove = nullptr;
    };
    
    bonjour_register(const char *name,
                     const char *regtype,
                     const char *domain,
                     uint16_t port,
                     notify_type notify = notify_type())
    : bonjour_named(name, regtype, domain)
    , m_port(port)
    , m_notify(notify)
    {}
    
    bonjour_register(bonjour_register const& rhs) = delete;
    bonjour_register(bonjour_register const&& rhs) = delete;
    void operator = (bonjour_register const& rhs) = delete;
    void operator = (bonjour_register const&& rhs) = delete;
    
    bool start()
    {
        return spawn(this, name(), regtype(), domain(), nullptr, m_port, 0, nullptr);
    }
    
private:
    
    void reply(DNSServiceFlags flags, const char *name, const char *regtype, const char *domain)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;
        
        if (flags & kDNSServiceFlagsAdd)
            notify(m_notify.m_add, this, name, regtype, domain, complete);
        else
            notify(m_notify.m_remove, this, name, regtype, domain, complete);
    }
    
    uint16_t m_port;

    notify_type m_notify;
};

#endif /* BONJOUR_REGISTER_HPP */
