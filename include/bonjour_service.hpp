
/**
 * @file bonjour_service.hpp
 * @brief Class for resolving Bonjour services.
 *
 * This file defines the `bonjour_service` class, which is responsible for resolving
 * a named Bonjour service. It extends the `bonjour_named` class, allowing the
 * retrieval of detailed information about a service, such as its network address
 * and port. The class provides callback mechanisms for handling resolution events,
 * including stopping and resolving services within a network.
 */

#ifndef BONJOUR_SERVICE_HPP
#define BONJOUR_SERVICE_HPP

#include "bonjour_named.hpp"

// An object for resolving a named bonjour service
// This can be constructed from separate names, or a bonjour_named object

/**
 * @brief Represents a Bonjour service, inheriting from the `bonjour_named` class.
 *
 * The `bonjour_service` class extends the functionality of the `bonjour_named` class,
 * adding support for managing and resolving Bonjour services. This class provides
 * methods to interact with Bonjour, such as resolving service details, handling
 * notifications, and managing service information like hostnames and port numbers.
 */

class bonjour_service : public bonjour_named
{
public:
    
    static constexpr auto service = DNSServiceResolve;
    
    /**
     * @brief Defines a type alias for the DNS service resolution callback function.
     *
     * This `using` directive creates an alias `callback` for the `DNSServiceResolveReply` type,
     * which represents the function signature for handling DNS service resolution replies.
     * It is used in Bonjour service resolution processes to respond to service discovery events.
     */
    
    using callback = DNSServiceResolveReply;
    
    /**
     * @brief Defines a type alias for a callback type specific to the `bonjour_service` class.
     *
     * This `using` directive creates an alias `callback_type` by using the `make_callback_type`
     * template. It generates a specific callback type for the `bonjour_service` class,
     * tailored to handle service-related events. The template parameters (`3, 1, 4, 5, 6`)
     * specify the exact arguments or structure of the callback, allowing for flexibility
     * in how the callback handles different aspects of service resolution or notifications.
     *
     * This alias helps simplify and manage the definition of complex callback functions
     * within the context of Bonjour service handling.
     */
    
    using callback_type = make_callback_type<bonjour_service, 3, 1, 4, 5, 6>;
    
    friend callback_type;
    
    /**
     * @brief Represents a notification type for the Bonjour service.
     *
     * The `notify_type` struct defines a callback mechanism that allows the Bonjour service
     * to notify interested parties about various events or state changes. It holds a function
     * pointer that can be used to handle notifications, making it flexible for different use cases.
     */
    
    struct notify_type
    {
        notify_type() : m_stop(nullptr), m_resolve(nullptr) {}

        bonjour_notify<bonjour_service>::stop_type m_stop = nullptr;
        bonjour_notify<bonjour_service>::resolve_type m_resolve = nullptr;
    };
    
    /**
     * @brief Constructs a Bonjour service object with a specified name and notification type.
     *
     * This constructor initializes a Bonjour service instance with the given service name
     * and an optional notification callback. The service is set up to listen for Bonjour
     * service advertisements, and the notification callback can be used to respond to events.
     *
     * @param named The name of the Bonjour service to be advertised or discovered.
     * @param notify An optional notification callback of type `notify_type` for handling service-related events.
     *               If no callback is provided, a default-constructed `notify_type` is used.
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
     * @brief Constructs a Bonjour service object with detailed parameters and a notification type.
     *
     * This constructor initializes a Bonjour service instance with the given service name,
     * registration type, domain, and an optional notification callback. It forwards these
     * parameters to another constructor of the `bonjour_service` class, which handles the
     * initialization.
     *
     * @param name The name of the Bonjour service to be advertised or discovered.
     * @param regtype The registration type of the service, typically representing the service's protocol.
     * @param domain The domain in which the Bonjour service is advertised or searched.
     *               If the domain is nullptr, the default domain is used.
     * @param notify An optional notification callback of type `notify_type` for handling service-related events.
     *               If no callback is provided, a default-constructed `notify_type` is used.
     */
    
    bonjour_service(const char *name, const char *regtype, const char *domain, notify_type notify = notify_type())
    : bonjour_service(bonjour_named(name, regtype, domain), notify)
    {}
    
    /**
     * @brief Copy constructor for the Bonjour service object.
     *
     * This constructor creates a new instance of the `bonjour_service` class by copying
     * the state of another `bonjour_service` object. The copied instance is initialized
     * with default (empty) service name, registration type, and domain. This constructor
     * is typically used for handling shallow copies or when the original object's specific
     * details do not need to be carried over.
     *
     * @param rhs The `bonjour_service` object to be copied. The copied instance will have
     *            its service name, registration type, and domain initialized to empty strings.
     */
    
    bonjour_service(bonjour_service const& rhs)
    : bonjour_named("", "", "")
    {
        *this = rhs;
    }
    
    /**
     * @brief Assignment operator for the Bonjour service object.
     *
     * This operator allows one `bonjour_service` object to be assigned the state of another.
     * It performs a deep copy of the state from the right-hand side (`rhs`) object into
     * the current object, replacing its existing state. This operator ensures that
     * both objects maintain separate instances of the service details.
     *
     * @param rhs The `bonjour_service` object to assign from. The current object will
     *            copy the service details (such as the name, registration type, and domain)
     *            from this object.
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
     * This method attempts to resolve the current Bonjour service, which involves querying
     * for more detailed information about the service such as its IP address and port number.
     * The resolution process typically interacts with the network to gather the necessary
     * details about the advertised service.
     *
     * @return `true` if the service resolution was successful and the service details were retrieved,
     *         `false` otherwise (e.g., if the service could not be resolved or an error occurred).
     */
    
    bool resolve()
    {
        return spawn(this, name(), regtype(), domain());
    }
    
    /**
     * @brief Retrieves the full name of the Bonjour service.
     *
     * This method constructs and returns the full name of the Bonjour service,
     * which typically includes the service name, registration type, and domain.
     * The full name uniquely identifies the service in the Bonjour network.
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
     * The hostname is typically resolved as part of the Bonjour service discovery process
     * and can be used to establish connections or further interactions with the service.
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
     * The port is typically resolved as part of the service discovery process and
     * can be used to establish network connections to the service.
     *
     * @return A `uint16_t` representing the port number of the Bonjour service.
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
     * This method processes the reply received during the resolution of the Bonjour service.
     * It is typically called when the service details (such as its full name, host, and port)
     * have been successfully retrieved. The method may update internal state or trigger actions
     * based on the received service information.
     *
     * @param flags A set of `DNSServiceFlags` that provide additional details about the response.
     *              These flags may indicate certain conditions, such as whether more replies are expected.
     * @param fullname The full name of the Bonjour service, typically in the format
     *                 "<ServiceName>.<RegType>.<Domain>".
     * @param host The hostname where the Bonjour service is running. This could be an IP address
     *             or a domain name.
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
    
    /**
     * @brief Stores the full name of the Bonjour service.
     *
     * This member variable holds the full name of the Bonjour service, which typically
     * includes the service name, registration type, and domain. The full name uniquely
     * identifies the service within the Bonjour network and is used during service
     * discovery and resolution.
     */
    
    std::string m_fullname;
    
    /**
     * @brief Stores the hostname of the Bonjour service.
     *
     * This member variable holds the hostname associated with the Bonjour service,
     * which may be a domain name or an IP address. The hostname is used to establish
     * connections to the machine where the Bonjour service is running and is typically
     * resolved during the service discovery process.
     */
    
    std::string m_host;
    
    /**
     * @brief Stores the port number of the Bonjour service.
     *
     * This member variable holds the port number on which the Bonjour service is running.
     * The port is used to establish network connections to the service once it has been
     * discovered and resolved. It is typically obtained during the service resolution process.
     */
    
    uint16_t m_port;
    
    /**
     * @brief Stores the notification callback for the Bonjour service.
     *
     * This member variable holds a `notify_type` object, which contains the callback function
     * to be invoked when certain events or changes occur in the Bonjour service. The callback
     * allows for customized handling of these notifications, such as responding to service
     * updates or changes in status.
     */
    
    notify_type m_notify;
};

#endif /* BONJOUR_SERVICE_HPP */
