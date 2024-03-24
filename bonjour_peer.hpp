
#ifndef BONJOUR_PEER_HPP
#define BONJOUR_PEER_HPP

#include "bonjour_browse.hpp"
#include "bonjour_register.hpp"
#include "bonjour_service.hpp"

#include <list>

// Options for bonjour_peer

struct bonjour_peer_options
{
    enum class modes { browse_only, register_only, both };

    modes m_mode = modes::both;
    bool m_self_discover = false;
};

// An object that is a peer service (and so offers both registration and browsing)
// This object resolves peers and assumes you will poll externally when required
// It has no notification facilities

class bonjour_peer
{
public:
        
    bonjour_peer(const char *name,
                 const char *regtype,
                 const char *domain,
                 uint16_t port,
                 bonjour_peer_options options = bonjour_peer_options())
    : m_options(options)
    , m_register(name, regtype, domain, port)
    , m_browse(regtype, domain)
    , m_this_service(m_register)
    {
        m_this_service.resolve();
    }
    
    bool start()
    {
        switch (m_options.m_mode)
        {
            case bonjour_peer_options::modes::browse_only:
                return m_browse.start();
                
            case bonjour_peer_options::modes::register_only:
                return m_register.start();
                
            default:
                return m_register.start() && m_browse.start();
        }
    }
    
    void stop()
    {
        m_register.stop();
        m_browse.stop();
    }
    
    void clear()
    {
        m_browse.clear();
    }
    
    const char *name() const
    {
        return m_register.name();
    }
    
    const char *regtype() const
    {
        return m_register.regtype();
    }
    
    const char *domain() const
    {
        return m_register.domain();
    }
    
    uint16_t port() const
    {
        return m_register.port();
    }
    
    void resolve()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        for (auto it = m_peers.begin(); it != m_peers.end(); it++)
            it->resolve();
    }
    
    void resolve(const bonjour_named& service)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        auto it = service.find(m_peers);
        
        if (it != m_peers.end())
            it->resolve();
    }
    
    void list_peers(std::list<bonjour_service> &peers)
    {
        std::list<bonjour_named> services;

        std::unique_lock<std::mutex> lock(m_mutex);
        
        m_browse.list_services(services);
        
        // Make sure
        // 1 - only matching items are left in m_peers
        // 2 - only non-matching items are left in services
        
        for (auto it = m_peers.begin(); it != m_peers.end(); it++)
        {
            const auto jt = it->find(services);
            
            if (jt == services.end())
                it = --m_peers.erase(it);
            else
                services.erase(jt);
        }
        
        // Add any new items to the list (noting if self-discovery is allowed)
        
        for (auto it = services.begin(); it != services.end(); it++)
        {
            if (m_options.m_self_discover || !it->equal(m_register))
                m_peers.emplace_back(*it);
        }
        
        peers = m_peers;
    }
    
    std::string resolved_host() const
    {
        return m_this_service.host();
    }
    
private:
    
    bonjour_peer_options m_options;
    
    bonjour_register m_register;
    bonjour_browse m_browse;
    bonjour_service m_this_service;
    
    mutable std::mutex m_mutex;
    std::list<bonjour_service> m_peers;
};

#endif /* BONJOUR_PEER_HPP */
