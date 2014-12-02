#ifndef __VP_UTIL_INCLUDE_H__
#define __VP_UTIL_INCLUDE_H__

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

/**
* @Function: min()/max()
**/
template<typename T>
inline const T& MyMin(const T& _a, const T& _b)
{
    return _b < _a ? _b : _a;
}

template<typename T>
inline const T& MyMax(const T& _a, const T& _b)
{
    return _a < _b ? _b : _a;
}

template<typename T>
void Swap(T& _v1, T& _v2)
{
    T t = _v1;
    _v1 = _v2;
    _v2 = t;
}

/**
 * @Function: reference count
 **/
class CCountRef
{
public:
    CCountRef()
    {
        m_nRef = 0;         
    }

    int addRef() 
    { 
        INTERLOCKED_INCREMENT(&m_nRef); 
        return m_nRef;
    }

    bool releaseRef() 
    { 
        INTERLOCKED_DECREMENT(&m_nRef);
        if (m_nRef == 0)    // TODO: ”–ª•≥‚∑Áœ’
        {
            onDestroy();
            return true;
        }

        return false;
    }

    int getRefCount()
    {
        return m_nRef;
    }

protected:
    virtual ~CCountRef()  {}         // protect object from being deleted explicitly
    void    onDestroy() { delete this; }

private:
    atomic_t    m_nRef;     
};


/**
 * @Function: the base class to act as singleton
 **/
template <class T>
class ISingleton : public CCountRef
{
public:
    static T* GetInstance()
    {
        if (!m_instance)
        {
            boost::lock_guard<boost::mutex> guard(m_instMutex);
            if (!m_instance) 
                m_instance = new T;
            m_instance->addRef();
        }

        return m_instance;
    }

    void Free()
    {
        releaseRef();
    }

    void Destroy()
    {
        while (!releaseRef());
    }

protected:
    ISingleton() {}
    virtual ~ISingleton() {};
    virtual void onDestroy() { m_instance = NULL; }

private:
    static boost::mutex m_instMutex;
    static T* m_instance;
};

template<typename T> boost::mutex ISingleton<T>::m_instMutex; 
template<typename T> T* ISingleton<T>::m_instance = NULL;


#endif
