#include "common.h"
#include "platform.h"

#ifdef WIN32

void bzero(void* _src, size_t _size)
{
    memset(_src, 0, _size);
}

void bcopy(const void *_src, void *_dest, size_t _n)
{
    memcpy(_dest, _src, _n);
}

int inet_aton(char* _cp, struct in_addr* _in)
{
    if (!_cp || !_in)   return -1;

    int rc = inet_addr(_cp);
    if (rc == -1 && strcmp(_cp, "255.255.255.255") == 0)
        return 0;

    _in->s_addr = rc;
    return 1;
}

int gettimeofday(struct timeval *_tv, struct timezone *_tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (_tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        _tv->tv_sec  = (long)(t / 1000000);
        _tv->tv_usec = (long)(t % 1000000);
    }

    if (_tz) 
    {
        if (!tzflag) 
        {
            _tzset();
            tzflag++;
        }

         _get_timezone(&_tz->tz_minuteswest);
         _tz->tz_minuteswest /= 60;
         _get_daylight(&_tz->tz_dsttime);
    }

    return 0;
}


/**
 * @Function:
 * @Param:
 *  [out] _tmvalue, return the current time value
 * @Return
 *  -2, parameter is wrong
 *  0,
 **/
int gettimeofday(struct timeval *_tmvalue)
{
    if (!_tmvalue) return -2;

    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year      = wtm.wYear - 1900;
    tm.tm_mon       = wtm.wMonth - 1;
    tm.tm_mday      = wtm.wDay;
    tm.tm_hour      = wtm.wHour;
    tm.tm_min       = wtm.wMinute;
    tm.tm_sec       = wtm.wSecond;
    tm. tm_isdst    = -1;
    clock = mktime(&tm);
    _tmvalue->tv_sec = (long)clock;
    _tmvalue->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}

int settimeofday(struct timeval* _tv, struct timezone *_tz)
{
    FILETIME    pft;
    SYSTEMTIME  syst, syslocal;

    LONGLONG ll = Int32x32To64(_tv->tv_sec, 10000000) + 116444736000000000;
    pft.dwLowDateTime = (DWORD)ll;
    pft.dwHighDateTime = ll >> 32;
    pft.dwLowDateTime += _tv->tv_usec * 10;

    ::FileTimeToSystemTime(&pft, &syst);
    ::SystemTimeToTzSpecificLocalTime(NULL, &syst, &syslocal);

    bool v = ::SetSystemTime(&syslocal);
    ON_ERROR_LOG_LAST_ERROR_AND_DO(v, == , false, return -1);
    return 0;
}

#endif