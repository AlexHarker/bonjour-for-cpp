
/**
 * @file bonjour_peer.hpp
 * @brief Class for managing a Bonjour peer with both browsing and registration capabilities.
 *
 * This file defines the `bonjour_peer` class, which represents a Bonjour peer capable of both browsing for
 * services and registering its own services. The class provides options to configure the peer's mode of
 * operation (browse-only, register-only, or both) and handles service resolution. It assumes external
 * polling for updates and does not include built-in notification mechanisms.
 */

#ifndef BONJOUR_PEER_HPP
#define BONJOUR_PEER_HPP

#include "bonjour_browse.hpp"
#include "bonjour_register.hpp"
#include "bonjour_service.hpp"

#include <list>

/**
 * @struct bonjour_peer_options
 * @brief Contains configuration options for a Bonjour peer.
 *
 * The `bonjour_peer_options` struct holds options that can be used to configure
 * the behavior of a `bonjour_peer` object on construction.
 */

struct bonjour_peer_options
{
     /** @brief An enum class for setting the mode of operation of a `bonjour_peer` object. */

    enum class modes 
    { 
        browse_only,    /*!< indicates that a peer should browse only and not register a service. */
        register_only,  /*!< indicates that a should register a service, but not browse. */
        both            /*!< indicates that a should both register a service and browse. */
    };

    /** @brief Sets the mode of operation for a `bonjour_peeer` object. */

    modes m_mode = modes::both;

    /** @brief Sets whether the object should report the internally hosted service, or not. */

    bool m_self_discover = false;
};

/**
 * @class bonjour_peer
 * @brief Represents a Bonjour peer for peer-to-peer service discovery and communication.
 *
 * The `bonjour_peer` class allows a named service to be registered to a given registration type and domain,
 * while also discovering and resolving other services registered to the same domain and registration type.
 * A list of peers can be retrieved from the object.
 * 
 * @note This object provides no notification facilities and needs to be polled externally.
 */

class bonjour_peer
{
public:

    /**
     * @brief Constructs a Bonjour peer given the specified service details.
     *
     * This constructor initializes a `bonjour_peer` object representing a service with the given name,
     * registration type (regtype), domain, and port. It also allows optional configuration through the
     * `bonjour_peer_options` parameter.
     *
     * @param name The name of the registered service as a C-string.
     * @param regtype The registration type (regtype) of the service as a C-string.
     * @param domain The domain in which the service will be registered as a C-string.
     * @param port The network port on which the service is running as an unsigned 16-bit integer.
     * @param options (Optional) An instance of `bonjour_peer_options` to specify additional options for the peer.
     */
    
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
    
    /**
     * @brief Starts the Bonjour service for peer registration and discovery.
     *
     * This method initializes and starts the Bonjour service for the peer-to-peer network. This method registers
     * the internal service on the network, and/or starts browsing for other peers (depending on the options set).
     *
     * @return true if the service was successfully started and browsing was successfully started, otherwise false.
     */
    
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

    /**
     * @brief Stops the Bonjour service for peer-to-peer communication.
     *
     * This method stops the currently running Bonjour service and unregisters it from the network, ensuring
     * that the service is no longer discoverable by other peers. It also stops browsing for other peers.
     */

    void stop()
    {
        m_register.stop();
        m_browse.stop();
    }
    
    /**
     * @brief Clears the list of discovered services.
     *
     * This method safely clears the list of Bonjour services that have been discovered so far. It is 
     * typically called to reset the service list when the browsing is stopped, or in process.
     */
    
    void clear()
    {
        m_browse.clear();
    }
    
    /**
     * @brief Retrieves the name of the internal Bonjour service.
     *
     * This method returns the name of the Bonjour service as a C-string.
     *
     * @return The name of the Bonjour service as a C-string.
     */
    
    const char *name() const
    {
        return m_register.name();
    }
    
    /**
     * @brief Retrieves the registration type (regtype) of the internal Bonjour service.
     *
     * This method returns the registration type (regtype) of the Bonjour service.
     *
     * @return The registration type of the service as a C-string.
     */
    
    const char *regtype() const
    {
        return m_register.regtype();
    }
    
    /**
     * @brief Retrieves the domain of the internal Bonjour service.
     *
     * This method returns the domain in which the Bonjour service is registered.
     *
     * @return The domain of the service as a C-string.
     */
    
    const char *domain() const
    {
        return m_register.domain();
    }
    
    /**
     * @brief Retrieves the port number on which the internal Bonjour service is running.
     *
     * This method returns the port number that was specified during the construction of the
     * bonjour_peer` object.
     * 
     * @return The port number as an unsigned 16-bit integer.
     */
    
    uint16_t port() const
    {
        return m_register.port();
    }
   
    /**
     * @brief Resolves the discovered Bonjour services to obtain details about peers.
     *
     * This method performs a resolution of the Bonjour service, in order to retrive the hostname and port 
     * for each service.
     *
     * @note The method does not return a value, and results should be obtained by calling `list_peers()`.
     */
    
    void resolve()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        for (auto it = m_peers.begin(); it != m_peers.end(); it++)
            it->resolve();
    }
    
    /**
     * @brief Resolves the specified Bonjour service internally if it is one of the discovered peers.
     *
     * This method first attemppts to find the named service in the internal list of discovered peers. If it
     * is found then it performs a resolution of the specified Bonjour service, in order to retrive the 
     * hostname and port 
     *
     * @param service A reference to a `bonjour_named` object representing the Bonjour
     * service to be resolved.
     *
     * @note The method does not return a value, and results should be obtained by calling `list_peers()`.
     */
    
    void resolve(const bonjour_named& service)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        auto it = service.find(m_peers);
        
        if (it != m_peers.end())
            it->resolve();
    }
    
    /**
     * @brief Lists all discovered Bonjour peers.
     *
     * This method is thread-safe and allows retrieval of all peers currently discovered.
     *
     * @param peers A reference to a list that will be populated with the details of the discovered peers.
     *
     * @note The list is cleared before being populated with the current set of discovered peers.
     */
    
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
    
    /**
     * @brief Retrieves the resolved hostnane of the internal Bonjour service.
     *
     * This method returns the hostnane after resolution of the internal Bonjour service.
     * 
     * @return The hostname as a `std::string`.
     */

    std::string resolved_host() const
    {
        return m_this_service.host();
    }
    
private:

    /** @brief Stores the configuration options for the Bonjour peer. */
    
    bonjour_peer_options m_options;
    
    /** @brief Handles the registration of the internal Bonjour service. */
    
    bonjour_register m_register;
    
    /** @brief Handles the browsing and discovery of Bonjour services. */
    
    bonjour_browse m_browse;

    /** @brief Handles the hostname resolution of the internal Bonjour service. */

    bonjour_service m_this_service;

    /**  @brief Mutex used for synchronizing access to shared data. */
    
    mutable std::mutex m_mutex;

    /**  @brief A list of discovered peers. */

    std::list<bonjour_service> m_peers;
};

#endif /* BONJOUR_PEER_HPP */
