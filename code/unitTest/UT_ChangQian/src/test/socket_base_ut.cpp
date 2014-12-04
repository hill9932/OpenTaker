#include "network/socket_base.h"
#include "gtest/gtest.h"


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
};

TEST_F(CBaseSocketTestSync, WriteAndRead)
{

}