
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
    
    /**
     * @brief Constructs a bonjour_browse object to browse Bonjour services.
     *
     * This constructor initializes a `bonjour_browse` object with the specified
     * service registration type (`regtype`) and domain. It also sets up a notification
     * handler (`notify`) that will be used to signal service addition, removal, or
     * stop events during the browsing process.
     *
     * @param regtype The registration type of the Bonjour service to browse.
     * @param domain The domain in which the service resides.
     * @param notify (Optional) A struct of type `notify_type` that defines the
     * notification callbacks for service events. If not provided, a default
     * `notify_type` instance is used.
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
     * This method clears any existing services and initiates the browsing of
     * Bonjour services by spawning a new process with the specified service type
     * and domain.
     *
     * @return True if the browsing service starts successfully; otherwise, false.
     */
    bool start()
    {
        clear();
        return spawn(this, regtype(), domain());
    }
    
    /**
     * @brief Clears the list of discovered services.
     *
     * This method locks the mutex to ensure thread safety and then clears
     * the list of Bonjour services that have been discovered so far.
     * It is typically called to reset the service list before starting a new
     * browse operation or when the browsing is stopped.
     */
    void clear()
    {
        mutex_lock lock(m_mutex);
        m_services.clear();
    }
    
    /**
     * @brief Provides the current list of discovered Bonjour services.
     *
     * This method locks the mutex to ensure thread safety and then assigns
     * the current list of discovered Bonjour services to the provided reference.
     * The list includes all services discovered so far.
     *
     * @param services A reference to a list that will be populated with the current
     * list of discovered services.
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
     * This method is invoked in response to changes in Bonjour services. It checks the
     * provided flags to determine if a service has been added or removed. If the
     * `kDNSServiceFlagsAdd` flag is set, the service is added to the list of services
     * if it's not already present. If the service is being removed, it is erased from
     * the list if found. After updating the list, the corresponding notification
     * function (`m_notify.m_add` or `m_notify.m_remove`) is called.
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
    
    std::list<bonjour_named> m_services;
    
    notify_type m_notify;
};

#endif /* BONJOUR_BROWSE_HPP */
