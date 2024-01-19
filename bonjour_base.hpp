
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
        
        static bonjour_thread *start_service(DNSServiceRef sd_ref)
        {
            return new bonjour_thread(sd_ref);
        }
        
        void stop()
        {
            mutex_lock lock(m_mutex);
            m_invalid = true;
        }
        
    private:
                
        bonjour_thread(DNSServiceRef sd_ref)
        : m_sd_ref(sd_ref)
        , m_invalid(false)
        , m_error(false)
        , m_thread(do_loop, this)
        {}
        
        ~bonjour_thread()
        {
            if (m_thread.joinable())
                m_thread.detach();
        }
        
        static void do_loop(bonjour_thread *thread)
        {
            thread->loop();
        }
        
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
  
    bonjour_base(const char *regtype, const char *domain)
    : m_regtype(impl::validate_regtype(regtype))
    , m_domain(impl::validate_domain(domain))
    , m_thread(nullptr)
    {}
    
    ~bonjour_base()
    {
        stop();
    }
    
    bonjour_base(bonjour_base const& rhs)
    : m_thread(nullptr)
    {
        *this = rhs;
    }
    
    void operator = (bonjour_base const& rhs)
    {
        m_regtype = rhs.m_regtype;
        m_domain = rhs.m_domain;
    }
    
    void stop()
    {
        if (active())
        {
            mutex_lock lock(m_mutex);
            m_thread->stop();
            m_thread = nullptr;
        }
    }
    
    bool active() const
    {
        mutex_lock lock(m_mutex);
        return m_thread;
    }
    
    const char *regtype() const
    {
        return m_regtype.c_str();
    }
    
    const char *domain() const
    {
        return m_domain.c_str();
    }
    
protected:
    
    template <typename T, typename ...Args>
    static void notify(T func, Args...args)
    {
        if (func)
            func(args...);
    }
    
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
    
    template <typename T, typename ...Args>
    void stop_notify(T func, Args...args)
    {
        stop();
        notify(func, args...);
    }
    
    // Automatic callback handling
    
    template<class F, class T, size_t ...Idxs> struct callback_type
    {};
    
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
