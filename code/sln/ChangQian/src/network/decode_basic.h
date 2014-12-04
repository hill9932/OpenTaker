#ifndef __HILUO_DECODE_BASIC_INCLUDE_H__
#define __HILUO_DECODE_BASIC_INCLUDE_H__

#include "common.h"


struct PacketBasicAttr_t
{
    u_int64     ts;
    u_int32     wireLen;
    u_int32     pktLen;

    u_int32     seq;            
    u_int32     appType;
    u_int32     srcIP;                  // if v6, will be the sum of last two 4 bytes
    u_int32     dstIP;
    u_int16     srcPort;
    u_int16     dstPort;

    u_int8      protocol    : 1;        // 0 = tcp,  1 = udp
    u_int8      version     : 1;        // 0 = V4, 1 = V6
    u_int8      flagSYN     : 1;
    u_int8      flagACK     : 1;
    u_int8      flagFIN     : 1;
    u_int8      flagRST     : 1;
    u_int8      flagPUSH    : 1;        // tcp flags
    u_int8      flagURT     : 1;

    u_int8      ext[];

    bool isIp()     { return srcIP != 0; }
    bool isTcpUdp() { return srcPort != 0; }
    bool isTcp()    { return isTcpUdp() && protocol == 0; }
    bool isUdp()    { return isTcpUdp() && protocol != 0; }
    bool isIpv4()   { return isIp() && version == 0; }
    bool isIpv6()   { return isIp() && version != 0; }
    u_int8 getIpVersion() { return version == 0 ? 4 : 6; }
};

struct PacketMeta_t
{
    byte*       data;   
    u_int16     layer5payloadLen;   // application payload length
    u_int8      layer3Offset;       // IP header
    u_int8      layer4Offset;       // tcp/udp header
    u_int8      layer5Offset;       // application header

    PacketBasicAttr_t  basicAttr;
};


#endif
