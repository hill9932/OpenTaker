/**
 * @Function: 
 *  Include some useful function related to network
 * @Memo:
 *  Created by hill, 5/21/2014
 **/

#ifndef __HILUO_NET_UTIL_INCLUDE_H__
#define __HILUO_NET_UTIL_INCLUDE_H__

#include "common.h"
#include "protocol_header_l4.h"

namespace ChangQian
{
    char* L4proto2name(u_int32 _proto);
    bool IsIPAddress(char* _ipAddr) ;
    bool IsMcstIPAddress(u_int32 _address);
    bool IsBcstIPAddress(u_int32 _address);
    bool IsMcstMacAddress(const byte* _mac);
    bool IsBcstMacAddress(const byte* _mac);
    CStdString Addr2Str(u_int32 _address);
    CStdString Addr2Str(net_in6_addr& _address);
    u_int64 GetMacAddr(const char* _mac);
    net_in6_addr GetIpAddr(const char* _addr);   // support both v4 and v6
}

#endif
