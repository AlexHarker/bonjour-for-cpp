
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
    
    uint16_t m_port;

    notify_type m_notify;
};

#endif /* BONJOUR_REGISTER_HPP */
