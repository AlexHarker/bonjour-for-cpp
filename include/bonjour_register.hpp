
/**
 * @file bonjour_register.hpp
 * @brief Class for registering Bonjour services in a network.
 *
 * This file defines the `bonjour_register` class, which is responsible for registering a named Bonjour service on the 
 * network. It extends the `bonjour_named` class by adding the functionality to register the service. This class also 
 * handles callbacks related to the registration process, including stopping, adding, or removing services.
 */

#ifndef BONJOUR_REGISTER_HPP
#define BONJOUR_REGISTER_HPP

#include "bonjour_named.hpp"

/**
 * @class bonjour_register
 * @brief Manages the registration of a Bonjour service.
 *
 * This class is responsible for registering and managing a Bonjour service, including handling service callbacks
 * and notifying a user of changes to registration. It inherits from the `bonjour_named` class, which provides the
 * basic functionality for specifying a named Bonjour service.
 *
 */

class bonjour_register : public bonjour_named
{
public:

    /** @brief Stores the DNS service method as expected by `bonjour_base`. */

    static constexpr auto service = DNSServiceRegister;
    
    /** @brief Type definition for the callback function used during service registration. */
    
    using callback = DNSServiceRegisterReply;
    
    /** @brief Sets the callback type used by the class to handle service events as expected by `bonjour_base`. */

    using callback_type = make_callback_type<bonjour_register, 2, 1, 3, 4, 5>;

    friend callback_type;
    
    /**
     * @struct notify_type
     * @brief Structure used for passing notification functions for Bonjour registrations events.
     *
     * This structure holds the function pointers for the `bonjour_register` class to handle notifications
     * received when Bonjour services are added, removed, or the registration process is stopped.
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
         * @brief Callback of type bonjour_notify<bonjour_register>::stop_type used to inform the owner that Bonjour registation has been stopped.
         *
         * @see bonjour_notify
         */

        bonjour_notify<bonjour_register>::stop_type m_stop = nullptr;

        /**
         * @var m_add
         * @brief Callback of type bonjour_notify<bonjour_register>::state_type used to inform the owner that the service has been added.
         * 
         * @see bonjour_notify
         */

        bonjour_notify<bonjour_register>::state_type m_add = nullptr;

        /**
         * @var m_remove
         * @brief Callback of type bonjour_notify<bonjour_register>::state_type used to inform the owner that the registered service has been removed.
         *
         * @see bonjour_notify
         */

        bonjour_notify<bonjour_register>::state_type m_remove = nullptr;
    };
    
    /**
     * @brief Constructs a bonjour_register object.
     *
     * This constructor initializes the Bonjour service with the provided name, type, domain, and port,
     * and optionally accepts a notify_type struct to handle service-related notifications.
     *
     * @param name The name of the service.
     * @param regtype The type of service being registered.
     * @param domain The domain in which the service is registered.
     * @param port The port number on which the service will be available.
     * @param notify (Optional) Parameter for providing notification callbacks.
     */
    
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
    
    /**
     * @brief Starts the Bonjour service registration process.
     *
     * This method initiates the registration of the Bonjour service using the parameters
     * provided when the `bonjour_register` object was constructed. 
     * 
     * @return true if the service registration was successfully started, false otherwise.
     */
    
    bool start()
    {
        return spawn(this, name(), regtype(), domain(), nullptr, m_port, 0, nullptr);
    }
    
    /**
     * @brief Retrieves the port number on which the Bonjour service is running.
     *
     * This method returns the port number that was specified during the construction of the
     * bonjour_register` object.
     * 
     * @return The port number as an unsigned 16-bit integer.
     */
    
    uint16_t port() const
    {
        return m_port;
    }
    
private:

    /**
     * @brief Handles the reply from the DNSServiceRegister call.
     * 
     * This method is invoked in response to changes in the registration of Bonjour services. It checks the provided
     * flags to determine if a service has been added or removed and calls the corresponding notification function
     * (`m_notify.m_add` or `m_notify.m_remove`).
     *
     * @param flags Flags indicating the type of event (e.g., service added, service removed).
     * @param name The name of the service being registered or removed.
     * @param regtype The type of service being registered or removed.
     * @param domain The domain in which the service is registered or removed.
     */
    
    void reply(DNSServiceFlags flags, const char *name, const char *regtype, const char *domain)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;
        
        if (flags & kDNSServiceFlagsAdd)
            notify(m_notify.m_add, this, name, regtype, domain, complete);
        else
            notify(m_notify.m_remove, this, name, regtype, domain, complete);
    }
    
    /** @brief Stores the port number of the Bonjour service. */

    uint16_t m_port;

    /** @brief Stores the notification callback object. */
    
    notify_type m_notify;
};

#endif /* BONJOUR_REGISTER_HPP */
