#include "network/inetaddr.h"
#include "gtest/gtest.h"

using namespace ChangQian;

TEST(InetAddr_TestCase, Address_Test)
{
    u_int32 ip = 0x11121314;
    in_addr ipAddr;
    ipAddr.s_addr = ip;

    CInetAddr   addr1(ip);
    CInetAddr   addr2(ipAddr);
    CInetAddr   addr3(htonl(ip));
    ASSERT_FALSE(addr1 == addr2);
    ASSERT_TRUE(addr3 == addr2);

    const char* strAddr = "17.18.19.20:0";
    CInetAddr   addr4(strAddr);
    ASSERT_TRUE(addr1 == addr4);

    strAddr = "17.18.19.20:134";
    CInetAddr   addr5(strAddr);
    ASSERT_EQ(addr5.port(), 134);
    ASSERT_TRUE(addr5.ip() == "17.18.19.20");
    ASSERT_TRUE(addr5.toString() == strAddr);

}