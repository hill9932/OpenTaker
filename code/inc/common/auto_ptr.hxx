/**
* @Function:
*  This file define smart pointers, such as AutoFree, CAutoPtr
**/

#ifndef __VP_AUTOPTR_INCLUDE_H__
#define __VP_AUTOPTR_INCLUDE_H__

#include "util.hxx"

#define SAFE_DELETE(x) { if (x != NULL) delete x; x = NULL; }

template<class T>
void DeleteArray(T* _array)
{
    delete[] _array;
}

template<class T>
void Delete(T* _mem)
{
    SAFE_DELETE(_mem);
}

/**
 * Auto free the memory which is allocated by malloc()
 **/
typedef void (*_RELEASE_)(void*);

template <class T, class FUNC = _RELEASE_>
class AutoFree
{
public:
    explicit AutoFree(T* _mem = NULL, FUNC _func = free)
    {
        m_mem = _mem;
        m_func = _func;
    }

    ~AutoFree()
    {
        assert(m_func);
        if (m_mem)
        {
            m_func(m_mem);
            m_mem = NULL;
        }
    }

    T* Get()
    {
        return m_mem;
    }

    T* operator = (T* _mem)
    {
        if (m_mem)   m_func(m_mem);
        m_mem = _mem;
        return m_mem;
    }

    T& operator[] (int _index)
    {
        return m_mem[_index];
    }

    operator T*()
    {
        return m_mem;
    }

    operator const T*() const
    {
        return m_mem;
    }

    operator void*()
    {
        return m_mem;
    }

    bool operator !()
    {
        return m_mem != NULL;
    }

    T* operator->()
    {
        return m_mem;
    }

private:
    T*      m_mem;
    FUNC    m_func;
};


/**
 * @Function:	Auto pointer for ReferenceControl
 * @Memo: T must realize CCountRef interface
 */
template <class T> class CAutoPtr
{
public:
	CAutoPtr(T *_ptr = NULL) 
		: m_raw_ptr(_ptr)
	{
		if (m_raw_ptr)
			m_raw_ptr->addRef();
	}

	CAutoPtr(const CAutoPtr& _auto_ptr) 
		: m_raw_ptr(_auto_ptr.m_raw_ptr)
	{
		if (m_raw_ptr)
			m_raw_ptr->addRef();
	}

	~CAutoPtr() 
	{
		if (m_raw_ptr)
			m_raw_ptr->releaseRef();
	}

	CAutoPtr& operator= (const CAutoPtr& _auto_ptr) 
	{
		return (*this = _auto_ptr.m_raw_ptr);
	}

	CAutoPtr& operator= (T* _ptr) 
	{
		if (m_raw_ptr == _ptr)
			return *this;

		if (_ptr)
			_ptr->addRef();
		if (m_raw_ptr)
			m_raw_ptr->releaseRef();
		m_raw_ptr = _ptr;
		return *this;
	}

	operator void* () const 
	{
		return m_raw_ptr;
	}

	T* operator-> () const 
	{
		assert(m_raw_ptr);
		return m_raw_ptr;
	}

	T* get() const 
	{
		return m_raw_ptr;
	}

	T* paraIn() const 
	{
		return m_raw_ptr;
	}

    T* release()
	{
	    T * pPtr = m_raw_ptr;
	    m_raw_ptr = NULL;

		return pPtr;
	}

	T*& paraOut() 
	{
		if (m_raw_ptr) 
        {
			m_raw_ptr->releaseRef();
			m_raw_ptr = NULL;
		}
		return static_cast<T*&>(m_raw_ptr);
	}

	T*& paraInOut() 
	{
		return static_cast<T*&>(m_raw_ptr);
	}

	T& operator * () const 
	{
		assert(m_raw_ptr);
		return *m_raw_ptr;
	}

private:
	T *m_raw_ptr;
};


/**
 *
 **/
template<typename T>
class CSharedPtr
{
public:
    CSharedPtr() : m_raw_ptr(NULL)
    {
        m_count_ref = new CCountRef;
        m_count_ref->addRef();
    }

    CSharedPtr(T* _ptr)
        : m_raw_ptr(_ptr)
    {
        m_count_ref = new CCountRef;
        m_count_ref->addRef();
    }

    CSharedPtr(const CSharedPtr<T>& _share_ptr)
        : m_raw_ptr(_share_ptr.m_raw_ptr), m_count_ref(_share_ptr.m_count_ref)
    {
        m_count_ref->addRef();
    }

    ~CSharedPtr()
    {
        if(0 == m_count_ref->releaseRef() && m_raw_ptr)
            delete m_raw_ptr;
    }

    CSharedPtr<T>& operator= (const CSharedPtr<T>& _share_ptr)
    {
        if(this != &_share_ptr) 
        {
            if(0 == m_count_ref->releaseRef() && m_raw_ptr)
                delete m_raw_ptr;
            m_raw_ptr = _share_ptr.m_raw_ptr;
            m_count_ref = _share_ptr.m_count_ref;
            m_count_ref->addRef();
        }
        return *this;
    }

    T* get() const { return m_raw_ptr; }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

private:
    T* m_raw_ptr;
    CCountRef* m_count_ref;
};

#endif
