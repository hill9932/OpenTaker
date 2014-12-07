#include "network/socket_base.h"
#include "gtest/gtest.h"

using namespace ChangQian;

/**
* @Function: Use CHFile to do synchronous operation
**/
class CBaseSocketTestSync : public ::testing::Test
{
public:
    virtual void SetUp()
    {}

    virtual void TearDown()
    {}

protected:
    CSocketUdp  m_socket;
};

TEST_F(CBaseSocketTestSync, WriteAndRead)
{
    ASSERT_EQ(m_socket.open(true), 0);
}