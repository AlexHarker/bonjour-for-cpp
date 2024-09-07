
/**
 * @file bonjour_base.hpp
 * @brief Base class for managing Bonjour service interactions.
 *
 * This file defines the `bonjour_base` class, which provides the foundational
 * mechanisms for managing Bonjour services. It includes functionality for
 * handling service discovery, registration, and resolution in a network,
 * utilizing threads and mutexes for synchronization. The class also interacts
 * with the Bonjour API through DNS-SD and manages service notifications and states.
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

template <class T>
struct bonjour_notify
{
    using stop_type = void(*)(T *);
    using state_type = void(*)(T *, const char *, const char *, const char *, bool);
    using resolve_type = void(*)(T *, const char *, const char *, uint16_t, bool);
};

// A base object to store information about bonjour services and to interact with the API

class bonjour_base
{
protected:
    
    using mutex_type = std::recursive_mutex;
    using mutex_lock = std::lock_guard<mutex_type>;
    
private:
    
    // A self-deleting thread for processing bonjour replies
    
    class bonjour_thread
    {
    public:
        
        /**
         * @brief Starts a Bonjour service on a separate thread.
         *
         * This method initializes and starts a new thread to handle a Bonjour service using the provided
         * DNSServiceRef. It is typically used to manage network service discovery in a background process.
         *
         * @param sd_ref A reference to a DNSServiceRef that represents the Bonjour service.
         *               This reference is used to manage the service.
         *
         * @return A pointer to a bonjour_thread object that encapsulates the thread running the service.
         *         Returns nullptr if the service could not be started.
         */
        
        static bonjour_thread *start_service(DNSServiceRef sd_ref)
        {
            return new bonjour_thread(sd_ref);
        }
        
        /**
         * @brief Stops the Bonjour service.
         *
         * This method halts the running Bonjour service associated with the current object. It ensures
         * that the service is properly shut down and any associated resources are released.
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
         * starts a new thread to handle the Bonjour service. The internal flags for service validity
         * and error state are initialized, and the service loop is executed in a separate thread.
         *
         * @param sd_ref A reference to a DNSServiceRef that represents the Bonjour service to be managed
         *               by this thread. This reference is used for managing the network service discovery.
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
         * This destructor ensures that the Bonjour service is properly stopped and any resources
         * associated with the thread are released. It cleans up any remaining resources when a
         * bonjour_thread object is destroyed.
         */
        
        ~bonjour_thread()
        {
            if (m_thread.joinable())
                m_thread.detach();
        }
        
        /**
         * @brief Main loop function for handling the Bonjour service.
         *
         * This static method runs the main loop for managing the Bonjour service. It processes
         * events related to the Bonjour service in the context of the provided bonjour_thread object.
         * The loop continues running until the service is stopped or an error occurs.
         *
         * @param thread A pointer to the bonjour_thread object that is executing this loop.
         *               The method uses this object to manage the state and operation of the
         *               Bonjour service.
         */
        
        static void do_loop(bonjour_thread *thread)
        {
            thread->loop();
        }
        
        /**
         * @brief Executes the Bonjour service loop.
         *
         * This method runs the event processing loop for the Bonjour service. It continuously
         * handles incoming events and processes them as long as the service is active. The loop
         * operates until the service is either stopped manually or an error condition is encountered.
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
               
        DNSServiceRef m_sd_ref;
        bool m_invalid;
        bool m_error;
        mutex_type m_mutex;
        std::thread m_thread;
    };
    
public:
  
    /**
     * @brief Constructs a bonjour_base object with the specified service type and domain.
     *
     * This constructor initializes a bonjour_base object using the provided service type
     * (regtype) and domain. It validates the service type and domain using internal validation
     * methods before assigning them to the object's member variables. The thread member is
     * initialized to nullptr, indicating that no service thread is currently active.
     *
     * @param regtype A C-string representing the service type to be registered with the Bonjour
     *                service. This string is validated before use.
     * @param domain  A C-string representing the domain in which the service will be registered.
     *                This string is validated before use.
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
     * associated with the bonjour_base object are released. If a service thread is running, it is
     * stopped and cleaned up to prevent resource leaks or dangling pointers.
     */
    
    ~bonjour_base()
    {
        stop();
    }
    
    /**
     * @brief Copy constructor for the bonjour_base class.
     *
     * This constructor creates a new bonjour_base object as a copy of an existing one.
     * It initializes the new object's service thread pointer to nullptr, meaning that
     * the service thread is not copied. The new object will need to establish its own
     * service thread if required.
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
     * This operator assigns the state of one bonjour_base object to another. It performs a deep copy
     * of the relevant members from the source object (`rhs`) to the target object (`this`).
     * The service thread is not copied; instead, the thread pointer is reset to nullptr, indicating
     * that the target object does not inherit an active service thread from the source object.
     * The target object will need to manage its own service thread separately.
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
     * This method halts any active Bonjour service associated with the current object.
     * It ensures that the service is properly shut down, any associated resources are
     * released, and the service thread is joined if it is still running. This prevents
     * resource leaks and ensures the orderly termination of the service.
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
     * This method returns a boolean value indicating whether the Bonjour service
     * associated with the current object is active and running. It is a const
     * method, meaning it does not modify the state of the object.
     *
     * @return `true` if the Bonjour service is active and running; `false` otherwise.
     */
    
    bool active() const
    {
        mutex_lock lock(m_mutex);
        return m_thread;
    }
    
    /**
     * @brief Retrieves the registered service type for the Bonjour service.
     *
     * This method returns a C-string representing the service type (regtype) that was registered
     * with the Bonjour service. The service type typically indicates the type of service being
     * advertised on the network (e.g., "_http._tcp."). This method is `const`, meaning it does
     * not modify the state of the object.
     *
     * @return A C-string representing the registered service type.
     */
    
    const char *regtype() const
    {
        return m_regtype.c_str();
    }
    
    /**
     * @brief Retrieves the domain in which the Bonjour service is registered.
     *
     * This method returns a C-string representing the domain where the Bonjour service
     * is registered. The domain typically specifies the network domain in which the
     * service is advertised (e.g., "local."). This method is `const`, meaning it does
     * not modify the state of the object.
     *
     * @return A C-string representing the domain of the registered service.
     */
    
    const char *domain() const
    {
        return m_domain.c_str();
    }
    
protected:
    
    /**
     * @brief Invokes a function or callable object with the provided arguments.
     *
     * This static template method is used to notify or invoke a function, lambda, or any
     * callable object with a set of arguments. The function or callable object (`func`)
     * is called with the provided arguments (`args...`). This method provides a flexible
     * mechanism to trigger callbacks or functions with varying signatures.
     *
     * @tparam T The type of the function or callable object to be invoked.
     * @tparam Args The types of the arguments to be passed to the function.
     *
     * @param func The function or callable object to be invoked.
     * @param args The arguments to be passed to the function. These arguments are
     *             forwarded to the callable object.
     */
    
    template <typename T, typename ...Args>
    static void notify(T func, Args...args)
    {
        if (func)
            func(args...);
    }
    
    /**
     * @brief Spawns a new thread to execute a member function of an object.
     *
     * This template method starts a new thread that invokes a member function of a given
     * object with the provided arguments. It allows the user to run the specified function
     * in a separate thread, passing the necessary arguments to the function.
     *
     * @tparam T The type of the object whose member function will be invoked.
     * @tparam Args The types of the arguments to be passed to the member function.
     *
     * @param object A pointer to the object whose member function will be executed.
     * @param args The arguments to be passed to the member function. These arguments
     *             are forwarded to the function when the thread is spawned.
     *
     * @return `true` if the thread was successfully spawned and the function is running;
     *         `false` otherwise.
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
     * @brief Notifies a function or callable object to stop a running service or operation.
     *
     * This template method invokes a specified function, lambda, or callable object with the
     * provided arguments to signal the stopping or termination of a service or operation.
     * It is typically used to cleanly shut down processes or services that are running in
     * the background.
     *
     * @tparam T The type of the function or callable object to be invoked.
     * @tparam Args The types of the arguments to be passed to the function.
     *
     * @param func The function or callable object to be notified for stopping the service
     *             or operation.
     * @param args The arguments to be passed to the function. These arguments are forwarded
     *             to the callable object to customize the stop notification.
     */
    
    template <typename T, typename ...Args>
    void stop_notify(T func, Args...args)
    {
        stop();
        notify(func, args...);
    }
    
    // Automatic callback handling
    
    /**
     * @brief A template struct for handling callback types with a specific function, object, and argument indices.
     *
     * This template struct is designed to encapsulate a callback type that associates a function or callable object (`F`),
     * an object or class type (`T`), and a list of argument indices (`Idxs...`). It is typically used in template metaprogramming
     * to create or manage callback types that involve invoking a member function on an object with specific arguments.
     *
     * @tparam F The type of the function or callable object to be associated with the callback.
     * @tparam T The type of the object on which the function will be called.
     * @tparam Idxs A parameter pack representing the indices of the arguments to be passed to the function.
     */
    
    template<class F, class T, size_t ...Idxs> struct callback_type
    {};
    
    /**
     * @brief A specialized template struct for handling callbacks with a specific function pointer, object, and argument indices.
     *
     * This specialization of the `callback_type` template struct is designed to handle callbacks that involve a function pointer with a specific
     * return type (`R`) and arguments (`Args...`). The struct also associates the function with an object of type `T` and uses a list of argument
     * indices (`ErrIdx`, `Idxs...`) to indicate which arguments are passed to the function. This specialization is particularly useful for managing
     * function pointers that need to be invoked on an object with specific argument handling, including error handling through the `ErrIdx`.
     *
     * @tparam R The return type of the function pointer.
     * @tparam Args The types of the arguments that the function pointer accepts.
     * @tparam T The type of the object on which the function pointer will be called.
     * @tparam ErrIdx The index of the argument used for error handling or special processing.
     * @tparam Idxs A parameter pack representing the indices of the arguments to be passed to the function.
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
    
    template <class T, size_t ErrIdx, size_t ...Idxs>
    using make_callback_type = callback_type<typename T::callback, T, ErrIdx, Idxs...>;
    
    mutable mutex_type m_mutex;
    
private:
    
    std::string m_regtype;
    std::string m_domain;
    
    bonjour_thread *m_thread;
};

#endif /* BONJOUR_BASE_HPP */
