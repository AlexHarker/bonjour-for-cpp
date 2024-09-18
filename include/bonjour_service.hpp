
/**
 * @file bonjour_service.hpp
 * @brief Class for resolving Bonjour services.
 *
 * This file defines the `bonjour_service` class, which is responsible for resolving a named Bonjour service.
 * It extends the `bonjour_named` class, allowing the retrieval of detailed information about a service,
 * including its hostname and port.
 */

#ifndef BONJOUR_SERVICE_HPP
#define BONJOUR_SERVICE_HPP

#include "bonjour_named.hpp"


/**
 * @brief Represents a Bonjour service to be resolved, inheriting from the `bonjour_named` class.
 *
 * The `bonjour_service` class extends the functionality of the `bonjour_named` class, adding support for
 * resolving the service on the network from its name. After resolution, it is possible to retrieve the
 * hostname and port on which the service is running.
 */

class bonjour_service : public bonjour_named
{
public:
    
    /** @brief Stores the DNS service method as expected by `bonjour_base`. */

    static constexpr auto service = DNSServiceResolve;
    
    /** @brief Type definition for the callback function used during service resolution. */
    
    using callback = DNSServiceResolveReply;
    
    /** @brief Sets the callback type used by the class to handle service events as expected by `bonjour_base`. */

    using callback_type = make_callback_type<bonjour_service, 3, 1, 4, 5, 6>;
    
    friend callback_type;
    
    /**
     * @struct notify_type
     * @brief Structure used for passing notification functions for Bonjour service events.
     *
     * This structure holds the function pointers for the `bonjour_serive` class to handle notifications
     * received when Bonjour services are resolved, or resolution is stopped.
     */
    
    struct notify_type
    {
        /**
         * @brief Constructs a default notify_type object with no callbacks.
         *
         * This constructor defaults all callbacks to `nullptr`.
         */

        notify_type() : m_stop(nullptr), m_resolve(nullptr) {}
        
        /**
         * @var m_stop
         * @brief Callback of type bonjour_notify<bonjour_serive>::stop_type used to inform the owner that service resolution has been topped.
         *
         * @see bonjour_notify
         */

        bonjour_notify<bonjour_service>::stop_type m_stop = nullptr;

        /**
         * @var m_resolve
         * @brief Callback of type bonjour_notify<bonjour_service>::resolve_type used to inform the owner that a service has been resolved.
         * 
         * @see bonjour_notify
         */

        bonjour_notify<bonjour_service>::resolve_type m_resolve = nullptr;
    };
    
    /**
     * @brief Constructs a Bonjour service object from a 'bonjour_named' object and an optional callback object.
     *
     * This constructor initializes a Bonjour service instance with the given service name and an optional
     * notification callback, which specifies functions to be called to respond to detected changes.
     *
     * @param named The name of the Bonjour service to be advertised or discovered.
     * @param notify An optional notification callback of type `notify_type` for handling service-related events.
     */
    
    bonjour_service(bonjour_named named, notify_type notify = notify_type())
    : bonjour_named(named)
    , m_port(0)
    , m_notify(notify)
    {
        if (strlen(name()))
            resolve();
    }
    
    /**
     * @brief Constructs a Bonjour service object from individual parameters and an optional callback object.
     *
     * This constructor initializes a Bonjour service instance with the given service name, registration type,
     * domain, and an optional notification callback. 
     *
     * @param name The name of the Bonjour service to be resolved.
     * @param regtype The registration type of the service (typically representing the service's protocol).
     * @param domain The domain in which the Bonjour service is advertised or searched.
     * @param notify An optional notification callback of type `notify_type` for handling service-related events.
     */
    
    bonjour_service(const char *name, const char *regtype, const char *domain, notify_type notify = notify_type())
    : bonjour_service(bonjour_named(name, regtype, domain), notify)
    {}
    
    /**
     * @brief Copy constructor for the Bonjour service object.
     *
     * Constructs a new instance of the `bonjour_service` class by copying the state of another `bonjour_service` object.
     *
     * @param rhs The `bonjour_service` object to be copied.
     */
    
    bonjour_service(bonjour_service const& rhs)
    : bonjour_named("", "", "")
    {
        *this = rhs;
    }
    
    /**
     * @brief Assignment operator for the Bonjour service object.
     *
     * Allows one `bonjour_service` object to be assigned the state of another.
     *
     * @param rhs The `bonjour_service` object to assign from.
     * @return A reference to the current `bonjour_service` object after assignment.
     */
    
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
    
    /**
     * @brief Resolves the Bonjour service details.
     *
     * This method attempts to resolve the current Bonjour service in order to retrive the hostname and port.
     *
     * @return `true` if  service resolution was started successful.
     * @note The results of the resolution are not returned should be queried manually from the object once
     * resolution is complete.
     */

    bool resolve()
    {
        return spawn(this, name(), regtype(), domain());
    }
    
    /**
     * @brief Retrieves the full name of the Bonjour service.
     *
     * This method returns the full name of the Bonjour service, which includes the service name, registration
     * type, and domain. The full name uniquely identifies the service in the Bonjour network.
     *
     * @return A `std::string` containing the full name of the Bonjour service.
     */
    
    std::string fullname() const
    {
        mutex_lock lock(m_mutex);
        std::string str(m_fullname);
        return str;
    }
    
    /**
     * @brief Retrieves the hostname associated with the Bonjour service.
     *
     * This method returns the hostname of the machine where the Bonjour service is running.
     *
     * @return A `std::string` containing the hostname of the Bonjour service.
     */
    
    std::string host() const
    {
        mutex_lock lock(m_mutex);
        std::string str(m_host);
        return str;
    }
    
    /**
     * @brief Retrieves the port number associated with the Bonjour service.
     *
     * This method returns the port number on which the Bonjour service is running.
     *
     * @return The port number as an unsigned 16-bit integer.
     */
    
    uint16_t port() const
    {
        mutex_lock lock(m_mutex);
        uint16_t port = m_port;
        return port;
    }
    
private:
    
    /**
     * @brief Handles the response from a Bonjour service resolution.
     *
     * This method processes the reply received during the resolution of the Bonjour service. It is called when the
     * service details (full name, host, and port) have been successfully retrieved. After updating the information
     * internally, the user is notified if callbacks have been set at construction.
     *
     * @param flags A set of `DNSServiceFlags` that provide details about the response.
     * @param fullname The full name of the Bonjour service.
     * @param host The hostname where the Bonjour service is running.
     * @param port The port number on which the Bonjour service is listening for connections.
     */
    
    void reply(DNSServiceFlags flags, const char *fullname, const char *host, uint16_t port)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;

        m_fullname = fullname;
        m_host = host;
        m_port = port;
                
        stop();
        
        notify(m_notify.m_resolve, this, fullname, host, port, complete);
    }
    
    /** @brief Stores the full name of the Bonjour service, including service name, registration type, and domain. */
    
    std::string m_fullname;
    
    /** @brief Stores the hostname of the Bonjour service. */
    
    std::string m_host;
    
    /** @brief Stores the port number of the Bonjour service. */
    
    uint16_t m_port;
    
    /** @brief Stores the notification callback object. */
    
    notify_type m_notify;
};

#endif /* BONJOUR_SERVICE_HPP */
