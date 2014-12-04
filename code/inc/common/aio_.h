#ifndef __HILUO_AIO_INCLUDE_H__
#define __HILUO_AIO_INCLUDE_H__

#include "common.h"

#ifdef LINUX
    #include "poll.h"
#endif

namespace LiangZhu
{
    class CAsyncIO
    {
    public:
        CAsyncIO();
        ~CAsyncIO();

        operator Aio_t() { return m_aioHandler; }

        /**
         * @Function: associate the handle with IOCP
         * @Param _flag: in windows, it specify the The maximum number of threads that
         *               the operating system can allow to concurrently process.
         *               in linux, it specifies the interested event type
         * @Return: NULL failed
         **/
        Aio_t   assoicateObject(handle_t _fd, void* _context = NULL, u_int32 _flag = 0);

    public:
        Aio_t   m_aioHandler;
    };
}

#endif
