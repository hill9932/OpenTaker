#include "time_.h"

namespace LiangZhu
{
    void MySleep(u_int64 _usec)
    {
        struct timeval timeout;
        fd_set  fdSet;
        int     fd = socket(AF_INET, SOCK_STREAM, 0);  //fileno(stdin);
        FD_ZERO(&fdSet);
        FD_SET(fd, &fdSet);

        timeout.tv_sec = (long)(_usec / 1000000);
        timeout.tv_usec = _usec % 1000000;

        if (select(fd + 1, &fdSet, NULL, NULL, &timeout) == 0) {}
        ::closesocket(fd);
    }

    u_int64 GetCurrentNsTime()
    {
        struct timeval tsVal;
        gettimeofday(&tsVal, NULL);
        return (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;
    }

    u_int32 GetSecondSinceToday(time_t _time)
    {
        if (0 == _time) return 0;

        struct tm tm;
        tm = *localtime(&_time);

        return tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    }

    u_int32 GetSecondOfToday()
    {
        time_t now = time(NULL);
        struct tm ltm = *localtime(&now);
        ltm.tm_hour = ltm.tm_min = ltm.tm_sec = 0;
        now = mktime(&ltm);

        return now;
    }

    CStdString CalcTimeDiff(TIME_INTERVAL& _start, TIME_INTERVAL& _end, int _count)
    {
        tchar elapse[128] = { 0 };
        TIME_INTERVAL freq;
        double fElap = 0;

#ifdef WIN32
        QueryPerformanceFrequency(&freq);
        fElap = (_end.QuadPart - _start.QuadPart) * 1000000.0 / freq.QuadPart;
#else
        fElap = 1000000.0 * (_end.tv_sec - _start.tv_sec) + _end.tv_usec - _start.tv_usec;
#endif

        if (1 == _count)
            sprintf_t(elapse, TEXT("%.3f us"), fElap);
        else
            sprintf_t(elapse, TEXT("%.3f, %.3f us"), fElap, fElap / _count);

        return elapse;
    }

    CStdString GetNowTimeStr()
    {
        return GetTimeString(time(NULL));
    }

    CStdString GetTimeString(time_t _time)
    {
        struct tm tm;
        tm = *localtime(&_time);

        CStdString r;
        r.Format("%4d.%02d.%02d %02d.%02d.%02d",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);
        return r;
    }

    CCmdTimer::CCmdTimer(const char* _msg)
    {
        if (_msg)   m_msg = _msg;
#ifdef WIN32
        QueryPerformanceFrequency(&m_freq);
#endif
        GetNowTime(m_startTime);
    }

    CCmdTimer::~CCmdTimer()
    {
        tchar elapse[128] = { 0 };
        TIME_INTERVAL now;
        double fElap = 0;

#ifdef WIN32
        QueryPerformanceCounter(&now);
        fElap = (now.QuadPart - m_startTime.QuadPart) * 1000000.0 / m_freq.QuadPart;
#else
        gettimeofday(&now, NULL);
        fElap = 1000000.0 * (now.tv_sec - m_startTime.tv_sec) + now.tv_usec - m_startTime.tv_usec;
#endif

        sprintf_t(elapse, TEXT("%.3f (us)"), fElap);
    }
}
