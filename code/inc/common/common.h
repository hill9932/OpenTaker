/**
 * @Function:
 *  This file includes the most often used macros, like error print.
 * @Memo:
 *  Created by hill, 4/11/2014
 **/

#ifndef __VP_COMMON_INCLUDE_H__
#define __VP_COMMON_INCLUDE_H__

#include "def.h"
#include "log.h"
#include "stdString.h"
#include "util.hxx"
#include "auto_ptr.hxx"


int     GetProgramBits();

/**
 * @Function: Get the error description specified by error code
 *  typically, error code would be errno in Linux and GetLastError() in Windows
 **/
CStdString  GetLastSysErrorMessage(u_int32 _errCode = 0, bool _herror = false);
u_int32     GetLastSysError(bool _herror = false);
void        SetLastSysError(u_int32 _errCode);
extern      CStdString GetFileName(const tchar* _path);


#define LOG_ERROR_MSG(errCode)          RM_LOG_ERROR(GetLastSysErrorMessage(errCode).c_str());
#define LOG_ERROR_MSG_S(errCode, msg)   RM_LOG_ERROR(msg << ": " << GetLastSysErrorMessage(errCode).c_str());
#define LOG_LAST_ERROR_MSG()            LOG_ERROR_MSG(0);

/**
 * If condition meets, print the error message according to the errno
 **/
#define ON_ERROR_RETURN_LAST_ERROR(expr, comp, error)   \
    if (expr comp error)    {\
        return GetLastSysError();   \
    }

// print the last error message
#define ON_ERROR_LOG_LAST_ERROR(expr, comp, error)	\
    if (expr comp error)	{\
        u_int32 err = GetLastSysError();   \
        LOG_ERROR_MSG(err)   \
    }

#define ON_ERROR_LOG_LAST_ERROR_AND_DO(expr, comp, error, action)    \
    if (expr comp error)	{\
        u_int32 err = GetLastSysError();   \
        LOG_ERROR_MSG(err);     \
        action;	\
    }

// log the specified message and do the action
#define ON_ERROR_LOG_MESSAGE_AND_DO(expr, comp, error, msg, action)    \
    if (expr comp error)	{\
        u_int32 err = GetLastSysError();   \
        LOG_ERROR_MSG_S(err, msg);  \
        action; \
    }

//
// Macros to define exceptions
//
#define DEFINE_EXCEPTION(className) class C##className##Exception : public std::exception {\
public:\
    C##className##Exception(const char* _msg){ \
    m_Msg = _msg;\
}\
    const char* what() const throw(){\
    return m_Msg.c_str();\
}\
    ~C##className##Exception() throw () {}\
private:\
    std::string m_Msg;\
};

#define THROW_EXCEPTION(className, Message)  throw C##className##Exception(Message);
#define EXCEPTION_NAME(className)	C##className##Exception

/**
 * exceptions
 */
DEFINE_EXCEPTION(InvalidParameter);
DEFINE_EXCEPTION(IOFailure);
DEFINE_EXCEPTION(Unavailable);
DEFINE_EXCEPTION(OutOfMemory);
DEFINE_EXCEPTION(UnImplemented);
DEFINE_EXCEPTION(NotFound);
DEFINE_EXCEPTION(MaximumReached);

struct ICallable
{
    virtual int operator()() = 0;
    virtual ~ICallable() {}
};


#endif
