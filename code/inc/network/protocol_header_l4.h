#ifndef __HILUO_PROTOCOL_HEADER_L4_INCLUDE_H__
#define __HILUO_PROTOCOL_HEADER_L4_INCLUDE_H__

#include "common.h"

// Host Order of ETH type value for Intel
const u_int16 HO_ETH_IP4        = 0x08;                
const u_int16 HO_ETH_IP6        = 0xDD86;           
const u_int16 HO_ETH_VLAN       = 0x081;            
const u_int16 HO_ETH_MPLS       = 0x4788;      
const u_int16 HO_ETH_MPLS_MT    = 0x4888;
const u_int16 HO_ETH_ARP        = 0x0608;
const u_int16 HO_ETH_PPPoE      = 0x6488;           
const u_int16 HO_GTP_U_V1_PORT  = 0x6808;        

// Network order
const u_int16 NT_ETH_IP4        = 0x0800;
const u_int16 NT_ETH_IP6        = 0x86DD;           
const u_int16 NT_ETH_VLAN       = 0x8100;            
const u_int16 NT_ETH_MPLS       = 0x8847;
const u_int16 NT_ETH_MPLS_MT    = 0x8848;   // MPLS multicast
const u_int16 NT_ETH_ARP        = 0x0806;
const u_int16 NT_ETH_PPPoE      = 0x8864;
const u_int16 NT_GTP_U_V1_PORT  = 0x0868;        

#pragma pack(push, 1)

struct ether_header_t
{
    u_int8  destHost[6];        // destination mac address
    u_int8  srcHost[6];	        // source mac address
    u_int16 type;		        // ethernet type

    u_int16 getEtherType() { return ntohs(type); }
};

struct ether_80211Q_t
{
    u_int16 vlanID;
    u_int16 protoType;
};

struct ipv4_header_t
{
    u_int8 	    ihl : 4;
    u_int8 	    ver : 4;
    u_int8   	tos;		    // service type
    u_int16  	len;	        // total len
    u_int16  	id;		        //
    u_int16  	fragOff;	    // offset
    u_int8   	ttl;		    // live time
    u_int8   	protocol;	    //
    u_int16  	checkSum;	    // check sum
    struct in_addr    srcAddr;	// source ip
    struct in_addr    dstAddr;	// destination ip
    u_int8      options[];

    u_int8  getVersion()        { return ver; }
    u_int8  getHeaderLen()      { return ihl << 2; }
    bool    isFragment()        { return (fragOff & 0xFF3F) != 0; }
};

struct net_in6_addr
{
    union
    {
        u_int8      addr8[16];
        u_int16     addr16[8];
        u_int32     addr32[4];
        u_int64     addr64[2];
    };

    bool operator > (const net_in6_addr& _v)
    {
        return memcmp(this, &_v, sizeof(net_in6_addr)) > 0;
    }
    bool operator < (const net_in6_addr& _v)
    {
        return memcmp(this, &_v, sizeof(net_in6_addr)) < 0;
    }
    bool operator >= (const net_in6_addr& _v)
    {
        return memcmp(this, &_v, sizeof(net_in6_addr)) >= 0;
    }
    bool operator <= (const net_in6_addr& _v)
    {
        return memcmp(this, &_v, sizeof(net_in6_addr)) <= 0;
    }
    bool operator == (const net_in6_addr& _v)
    {
        return memcmp(this, &_v, sizeof(net_in6_addr)) == 0;
    }
};

struct ipv6_header_t
{
    union
    {
        struct ipv6_header_ctl
        {
            u_int32     flow;
            u_int16     len;
            u_int8      next;
            u_int8      hlim;
        } ip6_un1;
        struct ipv6_header_ctl2
        {
            u_int8      tc1 : 4;
            u_int8      ver : 4;
            u_int8      fw1 : 4;
            u_int8      tc2 : 4;
            u_int16     fw2;
        } ip6_un2;
    } ip6_ctlun;

    net_in6_addr srcAddr;
    net_in6_addr dstAddr;
};

//total length : 20Bytes
struct tcp_header_t
{
    u_int16     srcPort;		// source port
    u_int16     dstPort;		// destination port
    u_int32     seqNo;		//
    u_int32     ackNo;		//

    union
    {
        struct field0_t
        {
            u_int8      nonce : 1;
            u_int8      reserved_1 : 3;
            u_int8      th1 : 4;		    // tcp header length
            u_int8      fin : 1;
            u_int8      syn : 1;
            u_int8      rst : 1;
            u_int8      psh : 1;
            u_int8      ack : 1;
            u_int8      urgent : 1;
            u_int8      echo : 1;
            u_int8      cwr : 1;
        } flags;
        u_int16 m_field0;
    } field0;

    u_int16     wndSize;		// 16 bit windows 
    u_int16     checkSum;		// 16 bits check sum
    u_int16     urgent;		    // 16 urgent p
    u_int8      options[];

    u_int8  getHeaderLen()      { return field0.flags.th1 << 2; }
    u_int32 getSeqNumber()      { return ntohl(seqNo); }
    u_int32 getAckNumber()      { return ntohl(ackNo); }
    u_int16 getWindowSize()     { return ntohs(wndSize); }
};

struct udp_header_t
{
    u_int16 srcPort;
    u_int16 dstPort;
    u_int16 pktLen;
    u_int16 checkSum;
};

#pragma pack(pop)

#endif
