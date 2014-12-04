#ifndef __HILUO_PROTOCOL_INCLUDE_H__
#define __HILUO_PROTOCOL_INCLUDE_H__

#include "common.h"

namespace ChangQian
{
    // every protocol has one structure
    struct L4Protocol_t
    {
        u_int32         id;
        CStdString      name;
        CStdString      desc;
        int             ports[10];
    };

    static
    const L4Protocol_t g_L4Map[] =
    {
        { 0, "TELNET", "TELNET", { 23, -1 } },
        {-1}
    };
}

#endif
