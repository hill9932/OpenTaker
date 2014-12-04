#include "math_.h"
#include "gtest/gtest.h"

using namespace LiangZhu;


/**
 * @Function: Test function of GetFileName()
 **/
TEST(Math_TestCase, GetAlignedValue_Test)
{
    for (int i = 0; i < ONE_MB; ++i)
    {
        ASSERT_TRUE(GetAlignedValue(i) % SECTOR_ALIGNMENT == 0);
        ASSERT_TRUE(GetAlignedValue(i) % SECTOR_ALIGNMENT == 0);
    }
}

/**
 * @Function: Test function of GetRandomNumber()
 **/
TEST(Math_TestCase, GetRandomNumber_Test)
{
    ASSERT_EQ(GetRandomNumber(0, 0), 0);
    ASSERT_EQ(GetRandomNumber(10, 0), -1);
    ASSERT_EQ(GetRandomNumber(10, 10), 10);

    int num = GetRandomNumber(1, 100);
    ASSERT_LE(num, 100);
    ASSERT_GE(num, 1);

    num = GetRandomNumber(3121, 45600);
    ASSERT_LE(num, 45600);
    ASSERT_GE(num, 3121);
}

/**
 * @Function: Test function of ChangeUnit()
 **/
TEST(Math_TestCase, ChangeUnit_Test)
{

}

/**
 * @Function: Test function of Sqrt2()
 **/
TEST(Math_TestCase, Sqrt2_Test)
{
    ASSERT_EQ(Sqrt2(0), 0);
    ASSERT_EQ(Sqrt2(1), 0);
    ASSERT_EQ(Sqrt2(2), 1);
    ASSERT_EQ(Sqrt2(3), 1);
    ASSERT_EQ(Sqrt2(4), 2);
    ASSERT_EQ(Sqrt2(5), 2);
    ASSERT_EQ(Sqrt2(6), 2);
    ASSERT_EQ(Sqrt2(7), 2);
    ASSERT_EQ(Sqrt2(8), 3);
    ASSERT_EQ(Sqrt2(9), 3);
    ASSERT_EQ(Sqrt2(29), 4);
    ASSERT_EQ(Sqrt2(32), 5);
    ASSERT_EQ(Sqrt2(63), 5);
    ASSERT_EQ(Sqrt2(64), 6);
    ASSERT_EQ(Sqrt2(1024), 10);
    ASSERT_EQ(Sqrt2(1025), 10);
}

/**
 * @Function: Test function of Distance()
 **/
TEST(Math_TestCase, Distance_Test)
{
    ASSERT_EQ(Distance(1, 1, 10), 0);
    ASSERT_EQ(Distance(1, 2, 10), 1);
    ASSERT_EQ(Distance(2, 1, 10), 9);
    ASSERT_EQ(Distance(-2, -1, 10), 1);
    ASSERT_EQ(Distance(-1, -2, 10), 9);
    ASSERT_EQ(Distance(0, 100, 100), 0);
    ASSERT_EQ(Distance(0, 200, 100), 0);
    ASSERT_EQ(Distance(100, 200, 101), 100);
    ASSERT_EQ(Distance(100, 200, 9), 1);
}
