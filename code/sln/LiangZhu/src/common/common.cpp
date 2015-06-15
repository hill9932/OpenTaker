#include "common.h"
#include "version.h"
#include "string_.h"

namespace LiangZhu
{
    const char** ErrorCode::errDesc_t = NULL;

    int GetProgramBits()
    {
        return sizeof(int*) * 8;
    }
    
    CStdString GetLibVersion()
    {
        CStdString version = PROJECT_NAME;
#if defined(DEBUG) || defined(_DEBUG)
        version += " DEBUG ";
#else
        version += "RELEASE ";
#endif
        version += HILUO_PACKAGE_VERSION;
        return version;
    }

    const char* GetErrorMessage(u_int32 _errCode)
    {
        return NULL;
    }

    /**
     * @Function: Get the last error code
     **/
    u_int32 GetLastSysError(bool _herror)
    {
        u_int32 errCode = 0;

#ifdef WIN32    
        errCode = _herror ? WSAGetLastError() : GetLastError();
#else
        errCode = _herror ? h_errno : errno;
#endif

        return errCode;
    }

    void SetLastSysError(u_int32 _errCode)
    {
        errno = _errCode;

#ifdef WIN32
        SetLastError(_errCode);
#endif
    }

    /**
     * @Function: Get the error description specified by error code
     *  typically, error code would be errno in Linux and GetLastError() in Windows
     *
     * @Param _errCode: the error code, mostly it is retrieved by GetLastSysError()
     * @Param _herror: indicates whether it is an network error
     **/
    CStdString GetLastSysErrorMessage(u_int32 _errCode, bool _herror)
    {
        string errMsg;
        if (0 == _errCode)
        {
#ifdef WIN32    
            _errCode = GetLastError();
#else
            _errCode = _herror ? h_errno : errno;
#endif
        }

        if (0 == _errCode)  return "";

#ifdef WIN32    
        LPSTR lpMsgBuf = NULL;

        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            _errCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0,
            NULL);

        if (lpMsgBuf)
        {
            errMsg = lpMsgBuf;
            LocalFree(lpMsgBuf);
        }
#else
        errMsg = strerror(_errCode);
#endif

        return errMsg;
    }

    CallTracer::CallTracer(const tchar* _FUNCTION, int _LINE)
    {
        RM_LOG_TRACE_S("CALL TRACER ** " << _FUNCTION << ":" << _LINE);
    }

    CallTracer::~CallTracer()
    {
    }

}
