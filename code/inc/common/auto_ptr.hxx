/**
* @Function:
*  This file define smart pointers, such as AutoFree, CAutoPtr
**/

#ifndef __HILUO_AUTOPTR_INCLUDE_H__
#define __HILUO_AUTOPTR_INCLUDE_H__

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
typedef void (*_RELEASE_VOID_)(void*);
typedef void (*_RELEASE_TCHAR_)(tchar*);
typedef void (*_RELEASE_CHAR_)(char*);
typedef void (*_RELEASE_BYTE_)(byte*);

template <class T, class FUNC>
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
template <class T, class FUNC> class CAutoPtr
{
public:
    CAutoPtr(T *_ptr = NULL, FUNC _func = NULL)
		: m_rawPtr(_ptr)
	{
        m_func = _func;
	}

	CAutoPtr(const CAutoPtr& _auto_ptr) 
		: m_rawPtr(_auto_ptr.m_rawPtr)
	{
	}

	~CAutoPtr() 
	{
        if (m_func && m_rawPtr)
            m_func(m_rawPtr);
	}

	CAutoPtr& operator= (const CAutoPtr& _auto_ptr) 
	{
		return (*this = _auto_ptr.m_rawPtr);
	}

	CAutoPtr& operator= (T* _ptr) 
	{
		if (m_rawPtr == _ptr)
			return *this;

		m_rawPtr = _ptr;
		return *this;
	}

	operator void* () const 
	{
		return m_rawPtr;
	}

    operator T* () const
    {
        return m_rawPtr;
    }

	T* operator-> () const 
	{
		assert(m_rawPtr);
		return m_rawPtr;
	}

	T* get() const 
	{
		return m_rawPtr;
	}

    T* release()
	{
	    T * pPtr = m_rawPtr;
	    m_rawPtr = NULL;

		return pPtr;
	}

	T& operator* () const 
	{
		assert(m_rawPtr);
		return *m_rawPtr;
	}

private:
	T *m_rawPtr;
    FUNC   m_func;
};


/**
 *
 **/
template<typename T>
class CSharedPtr
{
public:
    CSharedPtr() : m_rawPtr(NULL)
    {
        m_count_ref = new CCountRef;
        m_count_ref->addRef();
    }

    CSharedPtr(T* _ptr)
        : m_rawPtr(_ptr)
    {
        m_count_ref = new CCountRef;
        m_count_ref->addRef();
    }

    CSharedPtr(const CSharedPtr<T>& _share_ptr)
        : m_rawPtr(_share_ptr.m_rawPtr), m_count_ref(_share_ptr.m_count_ref)
    {
        m_count_ref->addRef();
    }

    ~CSharedPtr()
    {
        if(0 == m_count_ref->releaseRef() && m_rawPtr)
            delete m_rawPtr;
    }

    CSharedPtr<T>& operator= (const CSharedPtr<T>& _share_ptr)
    {
        if(this != &_share_ptr) 
        {
            if(0 == m_count_ref->releaseRef() && m_rawPtr)
                delete m_rawPtr;
            m_rawPtr = _share_ptr.m_rawPtr;
            m_count_ref = _share_ptr.m_count_ref;
            m_count_ref->addRef();
        }
        return *this;
    }

    T* get() const { return m_rawPtr; }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

private:
    T* m_rawPtr;
    CCountRef* m_count_ref;
};

#endif
