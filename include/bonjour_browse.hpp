
/**
 * @file bonjour_browse.hpp
 * @brief Class for browsing Bonjour services on a network.
 *
 * This file defines the `bonjour_browse` class, which allows a user to discover Bonjour services available
 * on the network. The class provides mechanisms for browsing services and maintains a list of discovered
 * services, but does not resolve their details.
 */

#ifndef BONJOUR_BROWSE_HPP
#define BONJOUR_BROWSE_HPP

#include "bonjour_base.hpp"
#include "bonjour_named.hpp"

#include <list>

/**
 * @class bonjour_browse
 * @brief Allows the user to browse Bonjour services on a network.
 *
 * The bonjour_browse class is used to discover available Bonjour services on a network. It extends the 
 * `bonjour_base` class and provides mechanisms to detect and list services but doesn't resolve their details.
 *
 * @see bonjour_base
 */

class bonjour_browse : public bonjour_base
{
public:

    /** @brief Stores the DNS service method as expected by `bonjour_base`. */

    static constexpr auto service = DNSServiceBrowse;
    
    /** @brief Type definition for the callback function used during service browsing. */
    
    using callback = DNSServiceBrowseReply;
    
    /** @brief Sets the callback type used by the class to handle service events as expected by `bonjour_base`. */
    
    using callback_type = make_callback_type<bonjour_browse, 3, 1, 4, 5, 6>;

    friend callback_type;
    
    /**
     * @struct notify_type
     * @brief Structure used for passing notification functions for Bonjour service events.
     *
     * This structure holds the function pointers for the `bonjour_browse` class to handle notifications
     * received when Bonjour services are added, removed, or browsing is stopped.
     */
    
    struct notify_type
    {
        /**
         * @brief Constructs a default notify_type object with no callbacks.
         *
         * This constructor defaults all callbacks to `nullptr`.
         */

        notify_type() : m_stop(nullptr), m_add(nullptr), m_remove(nullptr) {}
        
        /**
         * @var m_stop
         * @brief Callback of type bonjour_notify<bonjour_browse>::stop_type used to inform the owner that Bonjour browsing has been stopped.
         *
         * @see bonjour_notify
         */
        
        bonjour_notify<bonjour_browse>::stop_type m_stop;
        
        /**
         * @var m_add
         * @brief Callback of type bonjour_notify<bonjour_browse>::state_type used to inform the owner that a service has been added.
         * 
         * @see bonjour_notify
         */
        
        bonjour_notify<bonjour_browse>::state_type m_add;
        
        /**
         * @var m_remove
         * @brief Callback of type bonjour_notify<bonjour_browse>::state_type used to inform the owner that a service has been removed.
         *
         * @see bonjour_notify
         */
        
        bonjour_notify<bonjour_browse>::state_type m_remove;
    };
    
    /**
     * @brief Constructs a bonjour_browse object to browse Bonjour services.
     *
     * This constructor takes a specified service registration type (`regtype`) and domain. It also sets up a notification handler
     * (`notify`) that will be used to signal service addition, removal, or stop events during the browsing process.
     *
     * @param regtype The registration type of the Bonjour service to browse.
     * @param domain The domain in which the service resides.
     * @param notify (Optional) A struct of type `notify_type` that defines the notification callbacks for service events.
     */
    
    bonjour_browse(const char *regtype, const char *domain, notify_type notify = notify_type())
    : bonjour_base(regtype, domain)
    , m_notify(notify)
    {}
    
    bonjour_browse(bonjour_browse const& rhs) = delete;
    bonjour_browse(bonjour_browse const&& rhs) = delete;
    void operator = (bonjour_browse const& rhs) = delete;
    void operator = (bonjour_browse const&& rhs) = delete;
    
    /**
     * @brief Starts the browsing service for Bonjour.
     *
     * This method clears any existing services and initiates the browsing of Bonjour services by spawning 
     * a new thread with the specified service type and domain.
     *
     * @return true if the browsing service starts successfully; otherwise, false.
     */
    
    bool start()
    {
        clear();
        return spawn(this, regtype(), domain());
    }
    
    /**
     * @brief Clears the list of discovered services.
     *
     * This method safely clears the list of Bonjour services that have been discovered so far. It is 
     * typically called to reset the service list when the browsing is stopped, or in process.
     */
    
    void clear()
    {
        mutex_lock lock(m_mutex);
        m_services.clear();
    }
    
    /**
     * @brief Provides the current list of discovered Bonjour services.
     *
     * This method is thread-safe and allows retrieval of all services currently discovered.
     *
     * @param services A reference to a list that will be populated with the currently discovered services.
     * 
     * @note The list is cleared before being populated with the current set of discovered services.
     */
    
    void list_services(std::list<bonjour_named> &services)
    {
        mutex_lock lock(m_mutex);
        services = m_services;
    }
    
private:
    
    /**
     * @brief Handles Bonjour service callback responses for added or removed services.
     *
     * This method is invoked in response to changes in Bonjour services. It checks the provided flags to determine
     * if a service has been added or removed and updates the internally stored list accordingly. After this is done,
     * the corresponding notification function (`m_notify.m_add` or `m_notify.m_remove`) is called.
     *
     * @param flags Flags indicating the type of event (e.g., service added, service removed).
     * @param name The name of the service that is being added or removed.
     * @param regtype The registration type of the service.
     * @param domain The domain in which the service resides.
     */
    
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
    
    /** @brief Stores the list of currently discovered services interally. */

    std::list<bonjour_named> m_services;

    /** @brief Stores the notification callback object. */
    
    notify_type m_notify;
};

#endif /* BONJOUR_BROWSE_HPP */
