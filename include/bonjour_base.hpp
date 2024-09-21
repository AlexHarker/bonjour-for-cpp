
/**
 * @file bonjour_base.hpp
 * @brief Base class for managing Bonjour service interactions.
 *
 * This file defines the `bonjour_base` class, which provides the foundational mechanisms for managing Bonjour
 * services. It includes functionality for spawning threads to interact with the Bonjour API, for protecting
 * shared data for synchronization, and for managing service notifications and states.
 */

#ifndef BONJOUR_BASE_HPP
#define BONJOUR_BASE_HPP

#include <dns_sd.h>

#include "utils.hpp"

#include <cstring>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

/**
 * @brief Template struct that defines notification type related to Bonjour service events.
 *
 * The type definitions within this class are used to define common callback signatures for use with other 
 * objects, with the inclusion of a pointer to the owning object.
 * 
 * @tparam T The type of the object generating notifications.
 */

template <class T>
struct bonjour_notify
{
    /**
     * @brief Type alias for a notification that a Bonjour service has been stopped on an object of type `T`.
     * 
     * @param object A pointer to the object that has stopped its Bonjour service(s).
     */
    
    using stop_type = void(*)(T *);
    
    /**
     * @brief Type alias for notification that a service has been added or removed to an object of type `T`.
     * 
     * @param object A pointer to the object generating the notification.
     * @param name The name of the service as a C-string.
     * @param regtype The registration type of the service as a C-string.
     * @param domain The domain of the service as a C-string.
     * @param complete A flag indicating whether service updates are complete.
     */
    
    using state_type = void(*)(T *, const char *, const char *, const char *, bool);
    
    /**
     * @brief Type alias for a notification that a service has been resolved for an object of type `T`.
     *
     * @param object A pointer to the object generating the notification.
     * @param fullname The service's full name as a C-string.
     * @param hostname The hostname as a C-string.
     * @param port The port number where the resolved service is available.
     * @param complete A flag indicating whether service updates are complete.
     */
    
    using resolve_type = void(*)(T *, const char *, const char *, uint16_t, bool);
};

/**
 * @brief Base class for managing Bonjour service registration and operations.
 *
 * The `bonjour_base` class provides a generic interface managing Bonjour services asynchronously. It includes
 * methods for starting, stopping, and monitoring the status of the service, as well as handling underlying calls
 * to the Bonjour / DNS API. This class serves as a foundation for implementing Bonjour-based networking services.
 */

class bonjour_base
{
protected:
    
    /** @brief Type alias for a recursive mutex used to manage thread synchronization. */
    
    using mutex_type = std::recursive_mutex;
    
    /** @brief Type alias for a lock guard that manages the locking and unlocking of a recursive mutex. */
    
    using mutex_lock = std::lock_guard<mutex_type>;
    
private:
        
    /**
     * @brief Class for managing Bonjour service execution in a separate self-deleting thread.
     *
     * The `bonjour_thread` class is responsible for running a Bonjour service in its own thread. It manages the
     * lifecycle of the thread, including starting the service, processing events, and stopping the service when
     * required. This class is used to handle Bonjour operations asynchronously, ensuring non-blocking network
     * service discovery and advertisement.
     */
    
    class bonjour_thread
    {
    public:
        
        /**
         * @brief Starts a Bonjour service on a separate self-deleting thread.
         *
         * This method starts a new thread to handle a Bonjour service using the provided DNSServiceRef.
         *
         * @param sd_ref A reference to a DNSServiceRef that represents the Bonjour service.
         *
         * @return A pointer to a bonjour_thread object that encapsulates the thread running the service.
         * 
         * @note Ownership of the DNSServiceRef is transferred to the new thread object.
         */
        
        static bonjour_thread *start_service(DNSServiceRef sd_ref)
        {
            return new bonjour_thread(sd_ref);
        }
        
        /**
         * @brief Stops the Bonjour service.
         *
         * This method halts the running Bonjour service associated with the current object. It ensures that
         * the service is properly shut down and any associated resources are released.
         */
        
        void stop()
        {
            mutex_lock lock(m_mutex);
            m_invalid = true;
        }
        
    private:
        
        /**
         * @brief Constructs a bonjour_thread object and starts the Bonjour service in a new thread.
         *
         * This constructor initializes a bonjour_thread object with the provided DNSServiceRef and
         * starts a new thread running the service loop to handle the Bonjour service.
         *
         * @param sd_ref A  DNSServiceRef that represents the Bonjour service to be managed by this thread.
         */
        
        bonjour_thread(DNSServiceRef sd_ref)
        : m_sd_ref(sd_ref)
        , m_invalid(false)
        , m_error(false)
        , m_thread(do_loop, this)
        {}
        
        /**
         * @brief Destructor for the bonjour_thread class.
         *
         * This destructor ensures that all resources associated with the thread are released. 
         */
        
        ~bonjour_thread()
        {
            if (m_thread.joinable())
                m_thread.detach();
        }
        
        /**
         * @brief Main static loop function for handling the Bonjour service.
         *
         * This static method runs the main loop for managing the Bonjour service.
         *
         * @param thread A pointer to the bonjour_thread object that is executing this loop.
         */
        
        static void do_loop(bonjour_thread *thread)
        {
            thread->loop();
        }

        /**
         * @brief Executes the Bonjour service loop.
         *
         * This method runs the event processing loop for the Bonjour service. It continuously handles incoming
         * events and processes them until the service is either stopped manually or an error condition is 
         * encountered. On completion, the method deallocates the Bonjour service reference and self-deletes.
         */
        
        void loop()
        {
            bool exit = false;
        
            // Socket
            
            auto socket = DNSServiceRefSockFD(m_sd_ref);
            
            while (!exit)
            {
                auto rc = impl::wait_on_socket(socket, 1, 0);
                
                mutex_lock lock(m_mutex);
                
                if (m_invalid)
                    exit = true;
                else if (rc < 0 || (rc > 0 && DNSServiceProcessResult(m_sd_ref) != kDNSServiceErr_NoError))
                    m_error = true;
            }
            
            DNSServiceRefDeallocate(m_sd_ref);
            delete this;
        }
        
        /** @brief A reference to the Bonjour service instance. */
        
        DNSServiceRef m_sd_ref;
        
        /**  @brief A flag indicating whether the Bonjour service is in an invalid state. */
        
        bool m_invalid;
        
        /** @brief A flag indicating whether an error has occurred in the Bonjour service. */
        
        bool m_error;
        
        /** @brief A recursive mutex used to ensure thread-safe access to shared resources. */
        
        mutex_type m_mutex;
        
        /** @brief A thread object to manage the execution of the Bonjour service asyncnronously. */
        
        std::thread m_thread;
    };
    
public:
    
    /**
     * @brief Constructs a bonjour_base object with the specified service type and domain.
     * 
     * This constructor initializes a `bonjour_base` object using the provided registration type (regtype)
     * and domain. It validates these parameters before assigning them to the object's member variables.
     * 
     * @param regtype A C-string representing the service type to be registered with the Bonjour service.
     * @param domain  A C-string representing the domain in which the service will be registered.
     */

    bonjour_base(const char *regtype, const char *domain)
    : m_regtype(impl::validate_regtype(regtype))
    , m_domain(impl::validate_domain(domain))
    , m_thread(nullptr)
    {}
    
    /**
     * @brief Destructor for the bonjour_base class.
     *
     * This destructor ensures that any active Bonjour service is properly stopped and all resources
     * associated with the bonjour_base object are released. 
     */
    
    ~bonjour_base()
    {
        stop();
    }
    
    /**
     * @brief Copy constructor for the bonjour_base class.
     *
     * This constructor creates a new bonjour_base object as a copy of an existing one. The internal thread
     * is not copied, and the service is not started on the copy.
     *
     * @param rhs A reference to the bonjour_base object to be copied.
     */
    
    bonjour_base(bonjour_base const& rhs)
    : m_thread(nullptr)
    {
        *this = rhs;
    }
    
    /**
     * @brief Assignment operator for the bonjour_base class.
     *
     * This operator assigns the state of one bonjour_base object to another. The internal thread is not
     * copied, and the service is not started on the assigned copy.
     * 
     * @param rhs A reference to the bonjour_base object to be assigned to the target object.
     */
    
    void operator = (bonjour_base const& rhs)
    {
        m_regtype = rhs.m_regtype;
        m_domain = rhs.m_domain;
    }
    
    /**
     * @brief Stops the Bonjour service and cleans up resources.
     *
     * This method halts any active Bonjour service associated with the current object. It ensures that the
     * service is properly shut down and the internal thread is terminated and released.
     */
    
    void stop()
    {
        if (active())
        {
            // N.B. Don't hold the lock whilst stopping the thread as that can cause deadlocks
            
            m_thread->stop();
            mutex_lock lock(m_mutex);
            m_thread = nullptr;
        }
    }
    
    /**
     * @brief Checks if the Bonjour service is currently active.
     *
     * This method returns a boolean value indicating whether the Bonjour service associated with the current
     * object is active and running.
     *
     * @return `true` if the Bonjour service is active and running; `false` otherwise.
     */
    
    bool active() const
    {
        mutex_lock lock(m_mutex);
        return m_thread;
    }
    
    /**
     * @brief Retrieves the registration type (regtype) of the Bonjour service.
     *
     * This method returns the registration type (regtype) of the Bonjour service.
     *
     * @return The registration type of the service as a C-string.
     */

    const char *regtype() const
    {
        return m_regtype.c_str();
    }
    
    /**
     * @brief Retrieves the domain of the Bonjour service.
     *
     * This method returns the domain in which the Bonjour service is registered.
     *
     * @return The domain of the service as a C-string.
     */
    
    const char *domain() const
    {
        return m_domain.c_str();
    }
    
protected:
    
    /**
     * @brief Invokes a notification function with the provided arguments.
     *
     * This static template method is used to call a notification function with a variable set of arguments.
     *
     * @tparam T The type of the notification function.
     * @tparam Args The types of the arguments to be passed to the function.
     *
     * @param func The function or callable object to be invoked.
     * @param args The arguments to be forwarded to the notification function.
     */

    template <typename T, typename ...Args>
    static void notify(T func, Args...args)
    {
        if (func)
            func(args...);
    }
    
    /**
     * @brief Spawns a new thread to start the Bonjour service, initialising it with the provided arguments.
     *
     * This template method checks to see if the service is running, and if not, starts a new thread that 
     * invokes the calling object's service() member variable. This is expected to be a reference to a DNS
     * Service initialisation function in the Bonjour API that will construct the service, such as 
     * DNSServiceBrowse, DNSServiceRegister or DNSServiceResolve). It then starts an internally-hosted
     * self-deleting thread that will poll the newly constructed service.
     *
     * @tparam T The type of the object whose service member function will be invoked.
     * @tparam Args The types of the arguments to be passed to the initalisation function.
     *
     * @param object A pointer to the object whose hose service member function will be executed.
     * @param args The arguments to be passed to to the initalisation function. 
     *
     * @return `true` if the thread was successfully spawned and the service is running; `false` otherwise.
     */
    
    template <typename T, typename ...Args>
    bool spawn(T *object, Args ...args)
    {
        mutex_lock lock(m_mutex);

        // If the service is not active then attempt to spawn a thread for callbacks
        
        if (!active())
        {
            DNSServiceRef sd_ref = nullptr;
            auto err = T::service(&sd_ref, 0, 0, args..., T::callback_type::reply, object);

            if (err == kDNSServiceErr_NoError)
                m_thread = bonjour_thread::start_service(sd_ref);
            else
                stop();
        }
        
        return active();
    }

    /**
     * @brief Stops the service and then invoke a notification function with the provided arguments.
     *
     * This static template method is used to call a notification function with a variable set of arguments
     * after stopping the internal Bonjour service thread.
     *
     * @tparam T The type of the notification function.
     * @tparam Args The types of the arguments to be passed to the function.
     *
     * @param func The function or callable object to be invoked.
     * @param args The arguments to be forwarded to the notification function.
     */

    template <typename T, typename ...Args>
    void stop_notify(T func, Args...args)
    {
        stop();
        notify(func, args...);
    }
    
    // Automatic callback handling
    
    /**
     * @brief A template struct for assisting the generation of DNS callback handling code 
     *
     * This template struct is boilerplate that allows for a specialisation with can automatically deduce
     * the argument types of the DNS function. The actual implentation is provided in the specilaised template.
     *
     * @tparam F The type of the function to be associated with the callback.
     * @tparam T The type of the object that needs receive the reply from the DNS function.
     * @tparam Idxs A parameter pack representing the indices of arguments to be passed to the object on reply.
     */
    
    template<class F, class T, size_t ...Idxs>
    struct callback_type
    {};

    /**
     * @brief A specialized template struct for generating callback code for DNS functions.
     *
     * This specialization of the `callback_type` template struct is designed to handle callbacks from the DNS
     * API, check for errors, and call back into the owning object to execute service-specific code (in the
     * object's reply() method) in response to the call from the DNS API. The generated code passes on a set of
     * arguments (specified by index) to the reply() method of the owning object.
     *
     * @tparam R The return type of the DNS function (this is not used)
     * @tparam Args The types of the arguments for the DNS function.
     * @tparam T The type of the object that needs to receive the reply from the DNS function.
     * @tparam ErrIdx The index of the argument from the DNS function that contains error information.
     * @tparam Idxs A parameter pack representing the indices of arguments to be passed to the object on reply.
     */
    
    template<typename R, typename... Args, class T, size_t ErrIdx, size_t ...Idxs>
    struct callback_type<R(*)(Args...), T, ErrIdx, Idxs...>
    {
        static void reply(Args... args)
        {
            constexpr size_t context_idx = std::tuple_size<std::tuple<Args...>>::value - 1;
            
            std::tuple parameters(args...);
            
            T *obj = reinterpret_cast<T *>(std::get<context_idx>(parameters));
            
            if (std::get<ErrIdx>(parameters) == kDNSServiceErr_NoError)
            {
                mutex_lock lock(obj->m_mutex);
                obj->reply({std::get<Idxs>(parameters)}...);
            }
            else
                obj->stop_notify(obj->m_notify.m_stop, obj);
        }
    };

    /**
     * @brief Template alias for creating a callback type for a specified derived class.
     *
     * The `make_callback_type` alias simplifies the specification of a `callback_type` by automatically
     * deducing the function type (`T::callback`) of the DNS function used, and associating it with an object
     * of type `T`, an error handling index (`ErrIdx`), and a list of argument indices (`Idxs...`) to forward
     * on to its reply() method. 
     *
     * @tparam T The class type that contains the `callback` function.
     * @tparam ErrIdx The index of the argument used by the DNS function for error reporting.
     * @tparam Idxs The indices of the arguments to be passed to the reply method.
     */
    
    template <class T, size_t ErrIdx, size_t ...Idxs>
    using make_callback_type = callback_type<typename T::callback, T, ErrIdx, Idxs...>;
    
    /** @brief A mutable recursive mutex used for thread-safe access to shared resources. */
    
    mutable mutex_type m_mutex;
    
private:
    
    /** @brief A `std::string` representing the service registration type for the Bonjour service. */
    
    std::string m_regtype;
    
    /** @brief A `std::string` representing the domain in which the Bonjour service is registered. */
    
    std::string m_domain;
    
    /** @brief A pointer to a `bonjour_thread` object that manages the Bonjour service in a separate thread. */
    
    bonjour_thread *m_thread;
};

#endif /* BONJOUR_BASE_HPP */
