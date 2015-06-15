#include "var.h"
#include "gtest/gtest.h"

using namespace LiangZhu;


TEST(Variant_TestCase, Basic_Test)
{
    Variant var1, var2;
    var1 = Variant(100);
    EXPECT_TRUE(Variant(100) == var1);
    EXPECT_TRUE(var1 != Variant(99));
    EXPECT_FALSE(var1 != Variant(100));
    EXPECT_FALSE(var1 == Variant(99));

    var1 = Variant(-100);
    EXPECT_TRUE(var1 == Variant(-100));
    EXPECT_FALSE(var1 != Variant(-100));

    var1 = Variant(0);
    var2 = Variant(0);
    EXPECT_TRUE(var1 == var2);
    var2 = Variant((char)0);
    EXPECT_FALSE(var1 == var2);

    var1 = Variant("Hello, world!");
    EXPECT_TRUE(var1 == Variant("Hello, world!"));
    EXPECT_FALSE(var1 == Variant("Hello, World!"));

    var1 = Variant("Hello, World!");
    EXPECT_FALSE(var1 == Variant("Hello, world!"));
    EXPECT_TRUE(var1 == Variant("Hello, World!"));

    byte buf[] = { 0x1, 0x2, 0x45 };
    var1 = Variant(buf, sizeof(buf));
    var2.setValue(buf, sizeof(buf));
    EXPECT_TRUE(var1 == var2);

    var1 = Variant(char(2));
    char c = var1;
    EXPECT_TRUE(c == 2);
    EXPECT_TRUE(var1.size() == 1);

    var1 = Variant(byte(3));
    byte b = var1;
    EXPECT_TRUE(b == 3);
    EXPECT_TRUE(var1.size() == 1);

    var1 = Variant(int16(0xf501));
    int16 i16 = var1;
    EXPECT_TRUE(i16 == (int16)0xf501);
    EXPECT_TRUE(var1.size() == 2);

    var1 = Variant(u_int16(0xf502));
    u_int16 u16 = var1;
    EXPECT_TRUE(u16 == 0xf502);
    EXPECT_TRUE(var1.size() == 2);

    var1 = Variant(int32(0x8023f501));
    int32 i32 = var1;
    EXPECT_TRUE(i32 == 0x8023f501);
    EXPECT_TRUE(var1.size() == 4);

    var1 = Variant(u_int32(0x8023f501));
    u_int32 u32 = var1;
    EXPECT_TRUE(u32 == 0x8023f501);
    EXPECT_TRUE(var1.size() == 4);

    var1 = Variant(int64(-1));
    int64 i64 = var1;
    EXPECT_TRUE(i64 == -1);
    EXPECT_TRUE(var1.size() == 8);

    var1 = Variant(u_int64(-1));
    u_int64 u64 = var1;
    EXPECT_TRUE(u64 == -1);
    EXPECT_TRUE(var1.size() == 8);

    tchar* cs = "abce,@#490^&'-";
    var1 = Variant(cs);
    tchar* str = var1;
    EXPECT_TRUE(strcmp_t(var1, str) == 0);
    EXPECT_TRUE(var1.size() == strlen_t(cs));

    byte cb[] = {0x0, 0x1, 0x2, 0x3, 0x0, 0x14, 0x15, 0x24, 0x78, 0xa1, 0xaa, 0xf0, 0xff, 0x00};
    var1 = Variant(cb, sizeof(cb));
    byte* pb = var1;
    EXPECT_TRUE(0 == memcmp(pb, var1, sizeof(cb)));
    EXPECT_TRUE(var1.size() == sizeof(cb));
}