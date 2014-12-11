#include "net_util.h"
#include "string_.h"

namespace ChangQian
{
    char* L4proto2name(u_int32 _proto)
    {
        switch(_proto)
        {
        case 0:     return((char*)"IP");
        case 1:     return((char*)"ICMP");
        case 2:     return((char*)"IGMP");
        case 6:     return((char*)"TCP");
        case 17:    return((char*)"UDP");
        case 47:    return((char*)"GRE");
        case 112:   return((char*)"VRRP");
        default:    return((char*)"ERROR");
        }
    }

    bool IsIPAddress(char* _ipAddr)
    {
        if (!_ipAddr)   return false;

        struct in_addr addr4;
        struct in6_addr addr6;

        if (strchr(_ipAddr, ':') != NULL) // IPv6
        {
            if(inet_pton(AF_INET6, _ipAddr, &addr6) == 1)
                return true;
        }
        else
        {
            if(inet_pton(AF_INET, _ipAddr, &addr4) == 1)
                return true;
        }

        return false;
    }

    net_in6_addr GetIpAddr(const char* _addr)
    {
        struct in_addr addr4;
        struct in6_addr addr6;
        net_in6_addr    res = { 0 };

        if (strchr(_addr, ':') != NULL) // IPv6
        {
            if (inet_pton(AF_INET6, _addr, &addr6) == 1)
            {
                for (int i = 0; i < 16; ++i)
                    res.addr8[i] = addr6.s6_addr[i];
            }
        }
        else
        {
            if (inet_pton(AF_INET, _addr, &addr4) == 1)
                res.addr32[0] = ntohl(addr4.s_addr);
        }

        return res;
    }

    bool IsMcstIPAddress(u_int32 _address)
    {
        byte* p = (byte*)&_address;
        if (*p >= 224 && *p <= 239)
            return true;

        return false;
    }

    bool IsBcstIPAddress(u_int32 _address)
    {
        byte* p = (byte*)&_address;
        if (*(p + 3) == 255)
            return true;

        return false;
    }

    bool IsMcstMacAddress(const byte* _mac)
    {
        return (0x80 & *_mac) != 0;
    }

    bool IsBcstMacAddress(const byte* _mac)
    {
        const byte BCST_MAC_ADDR[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        return memcmp(_mac, BCST_MAC_ADDR, 6) == 0;
    }

    CStdString Addr2Str(u_int32 _address)
    {
        byte* p = (byte*)&_address;

        CStdString s;
        s.Format("%u.%u.%u.%u", *(p + 3), *(p + 2), *(p + 1), *p);
        return s;
    }

    CStdString Addr2Str(net_in6_addr& _address)
    {
        const byte IP4_MAP_PRE[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
        byte* p = (byte*)&_address;
        CStdString s;
        if (memcmp(p, IP4_MAP_PRE, sizeof(IP4_MAP_PRE)) == 0)
        {
            p += sizeof(IP4_MAP_PRE);
            s.Format("::FFFF:%u.%u.%u.%u", *p, *(p + 1), *(p + 2), *(p + 3));
        }
        else
        {
            char addrBuf[64];
            bool hasCompact = false;
            bool inCompact = false;
            int pos = 0;
            for (int i = 0; i < sizeof(net_in6_addr); i += 2)
            {
                u_int16 cur = ntohs(*(u_int16*)(p + i));
                bool needPrint = true;
                if (i == sizeof(net_in6_addr) - 2)
                {
                }
                else if (cur == 0)
                {
                    if (!hasCompact || inCompact)
                    {
                        if (!hasCompact)
                        {
                            hasCompact = true;
                            addrBuf[pos++] = ':';
                        }
                        inCompact = true;
                        needPrint = false;
                    }
                }
                else
                {
                    inCompact = false;
                }
                if (needPrint)
                {
                    if (i > 0)
                    {
                        addrBuf[pos++] = ':';
                    }
                    sprintf(&addrBuf[pos], "%X", cur);
                    pos = strlen(addrBuf);
                }
            }
            s = addrBuf;
        }

        return s;
    }

    u_int64 GetMacAddr(const char* _mac)
    {
        if (!_mac)  return -1;

        vector<CStdString> values;
        LiangZhu::StrSplit(_mac, ":-", values);
        if (values.size() != 6) return -1;

        u_int64 macAddr = 0;
        for (int i = 0; i < values.size(); ++i)
        {
            macAddr <<= 8;
            macAddr += LiangZhu::AtoX(values[i]);
        }
        return macAddr;
    }
}
