
/**
 * @file bonjour_register.hpp
 * @brief Class for registering Bonjour services in a network.
 *
 * This file defines the `bonjour_register` class, which is responsible for
 * registering a named Bonjour service. It extends the `bonjour_named` class
 * by adding the functionality to register the service with the DNSServiceRegister
 * API. This class also handles callbacks related to the registration process,
 * including stopping, adding, or removing services in the network.
 */

#ifndef BONJOUR_REGISTER_HPP
#define BONJOUR_REGISTER_HPP

#include "bonjour_named.hpp"

// An object for registering a named bonjour service

/**
 * @class bonjour_register
 * @brief Manages the registration of a Bonjour service.
 *
 * This class is responsible for registering and managing a Bonjour service, including
 * handling service callbacks and maintaining the service state. It inherits from the
 * `bonjour_named` class, which provides the basic functionality for managing a named
 * Bonjour service.
 *
 * The `bonjour_register` class extends `bonjour_named` by adding support for service
 * registration via DNSServiceRegister and handling additional notifications related to
 * the service lifecycle.
 */

class bonjour_register : public bonjour_named
{
public:

    static constexpr auto service = DNSServiceRegister;
    
    /**
     * @brief Defines a callback type for DNS service registration replies.
     *
     * This alias defines `callback` as the type `DNSServiceRegisterReply`, which is used
     * as the callback function type for handling replies from DNSServiceRegister.
     * The callback is invoked when the Bonjour service registration completes, and it
     * processes any relevant information returned by the DNS service.
     */
    
    using callback = DNSServiceRegisterReply;
    
    /**
     * @brief Defines a specialized callback type for the `bonjour_register` class.
     *
     * This alias defines `callback_type` as a type created by the `make_callback_type` template,
     * specifically tailored for the `bonjour_register` class. It constructs a callback type that
     * maps specific argument positions (2, 1, 3, 4, 5) when invoking the callback function, allowing
     * for flexible handling of the parameters received in service registration callbacks.
     *
     * The `callback_type` is used internally to manage the callbacks associated with the
     * Bonjour service registration process.
     */
    
    using callback_type = make_callback_type<bonjour_register, 2, 1, 3, 4, 5>;

    friend callback_type;
    
    /**
     * @struct notify_type
     * @brief Defines a structure to manage notification callbacks for the Bonjour service.
     *
     * The `notify_type` structure holds callback functions that are used to manage various
     * state changes related to the Bonjour service. It provides members for handling notifications
     * when the service is stopped, added, or removed. This structure is passed to the
     * `bonjour_register` class to handle these notifications during the service's lifecycle.
     */
    
    struct notify_type
    {
        notify_type() : m_stop(nullptr), m_add(nullptr), m_remove(nullptr) {}

        bonjour_notify<bonjour_register>::stop_type m_stop = nullptr;
        bonjour_notify<bonjour_register>::state_type m_add = nullptr;
        bonjour_notify<bonjour_register>::state_type m_remove = nullptr;
    };
    
    /**
     * @brief Constructs a bonjour_register object.
     *
     * @param name The name of the service.
     * @param regtype The type of service being registered.
     * @param domain The domain in which the service is registered.
     * @param port The port number on which the service will be available.
     * @param notify Optional parameter for providing notification callbacks, defaults to an empty notify_type.
     *
     * This constructor initializes the Bonjour service with the provided name, type, domain, and port,
     * and optionally accepts a notify_type struct to handle service-related notifications.
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
     * @return true if the service registration was successfully started, false otherwise.
     *
     * This method initiates the registration of the Bonjour service using the parameters
     * provided when the `bonjour_register` object was constructed. It returns a boolean
     * value indicating whether the registration process was successfully initiated.
     */
    
    bool start()
    {
        return spawn(this, name(), regtype(), domain(), nullptr, m_port, 0, nullptr);
    }
    
    /**
     * @brief Retrieves the port number on which the Bonjour service is running.
     *
     * @return The port number as an unsigned 16-bit integer.
     *
     * This method returns the port number that was specified during the construction of the
     * `bonjour_register` object. It provides a way to access the port on which the Bonjour
     * service is currently registered and running.
     */
    
    uint16_t port() const
    {
        return m_port;
    }
    
private:
    
    /**
     * @brief Handles the reply from the DNSServiceRegister call.
     *
     * @param flags Flags that provide additional information about the reply.
     * @param name The name of the service being registered.
     * @param regtype The type of service being registered.
     * @param domain The domain in which the service is registered.
     *
     * This method is invoked as a callback when the Bonjour service registration process
     * receives a reply. It processes the information returned by the DNS service, which
     * includes the flags, service name, type, and domain. This method typically handles
     * tasks such as confirming the registration status or updating internal state based on
     * the service registration results.
     */
    
    void reply(DNSServiceFlags flags, const char *name, const char *regtype, const char *domain)
    {
        bool complete = (flags & kDNSServiceFlagsMoreComing) == 0;
        
        if (flags & kDNSServiceFlagsAdd)
            notify(m_notify.m_add, this, name, regtype, domain, complete);
        else
            notify(m_notify.m_remove, this, name, regtype, domain, complete);
    }
    
    /**
     * @brief Stores the port number for the Bonjour service.
     *
     * This member variable holds the port number as a 16-bit unsigned integer (`uint16_t`)
     * on which the Bonjour service is registered and accessible. It is typically set during
     * the initialization of the `bonjour_register` object and used when starting the service.
     */
    
    uint16_t m_port;

    /**
     * @brief Holds the notification callbacks for the Bonjour service.
     *
     * This member variable stores an instance of `notify_type`, which contains callback
     * functions for handling various state changes of the Bonjour service. These include
     * callbacks for stopping, adding, and removing the service. The `m_notify` member is
     * used by the `bonjour_register` class to manage notifications during the service's lifecycle.
     */
    
    notify_type m_notify;
};

#endif /* BONJOUR_REGISTER_HPP */
