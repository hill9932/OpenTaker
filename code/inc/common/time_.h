#ifndef __HILUO_TIME_INCLUDE_H__
#define __HILUO_TIME_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    void MySleep(u_int64 _usec);

    /**
     * @Function: Return string format (year.month.day hour:minute:second) of current time
     **/
    CStdString GetNowTimeStr();

    /**
     * @Function: Convert a time_t value into a string format
     **/
    CStdString GetTimeString(time_t _time);

    /**
     * @Function: calculate the different value between two time interval
     **/
    CStdString CalcTimeDiff(TIME_INTERVAL& _start, TIME_INTERVAL& _end, int _count = 1);

    /**
    * @Function:¡¡Caculate the time different in ms
    **/
    u_int64 GetCurrentNsTime();

    /**
    * @Function: get the seconds since the 00:00:00 of the day
    **/
    u_int32 GetSecondSinceToday(time_t _time);

    /**
    * @Function: get the seconds from 00:00:00 of today to EPOC
    **/
    u_int32 GetSecondOfToday();

    /**
    * @Function: This class will print the current time when it is created and destroyed
    *  Often used when need to timer a command execution.
    **/
    class CCmdTimer
    {
    public:
        CCmdTimer(const char* _msg = NULL);
        ~CCmdTimer();

    private:
        CStdString      m_msg;
        TIME_INTERVAL   m_startTime;

#ifdef WIN32
        TIME_INTERVAL   m_freq;
#endif
    };

#define PRINT_EXEC_TIME(msg)    CCmdTimer timer##msg(#msg);

}

#endif

