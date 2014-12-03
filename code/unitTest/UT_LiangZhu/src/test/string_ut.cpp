#include "string_.h"
#include "gtest/gtest.h"


/**
 * @Function: Test function of StrSplit()
 **/
TEST(String_TestCase, StrSplit_Test)
{
    const tchar* testString = "10.1.23.5.";
    vector<CStdString>  v;
    ASSERT_EQ(StrSplit(testString, ".", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10..1..23..5...";
    ASSERT_EQ(StrSplit(testString, ".", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10.....1.......23.5....";
    ASSERT_EQ(StrSplit(testString, ".", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10;.1:23..5.;;;";
    ASSERT_EQ(StrSplit(testString, ".;:", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10\t\t1:23..\t5.;::";
    ASSERT_EQ(StrSplit(testString, ".;:\t", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "101235";
    ASSERT_EQ(StrSplit(testString, ".;:\t", v), 1);
    ASSERT_TRUE(v[0] == testString);
}

/**
 * @Function: Test function of StrSplit_s()
 **/
TEST(String_TestCase, StrSplit_s_Test)
{
    const tchar* testString = "10.1.23.5..";
    vector<CStdString>  v;
    ASSERT_EQ(StrSplit_s(testString, ".", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10.1.23.5.";
    ASSERT_EQ(StrSplit_s(testString, "..", v), 1);
    ASSERT_TRUE(v[0] == testString);

    v.clear();
    testString = "10..1..23..5...";
    ASSERT_EQ(StrSplit_s(testString, "..", v), 5);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");
    ASSERT_TRUE(v[4] == ".");

    v.clear();
    testString = "10...1...23...5....";
    ASSERT_EQ(StrSplit_s(testString, "..", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == ".1");
    ASSERT_TRUE(v[2] == ".23");
    ASSERT_TRUE(v[3] == ".5");

    v.clear();
    testString = "10....1....23....5......";
    ASSERT_EQ(StrSplit_s(testString, "..", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "1");
    ASSERT_TRUE(v[2] == "23");
    ASSERT_TRUE(v[3] == "5");

    v.clear();
    testString = "10.\t..1.\t.\t23..\t.\t.\t5";
    ASSERT_EQ(StrSplit_s(testString, ".\t", v), 4);
    ASSERT_TRUE(v[0] == "10");
    ASSERT_TRUE(v[1] == "..1");
    ASSERT_TRUE(v[2] == "23.");
    ASSERT_TRUE(v[3] == "5");
}

/**
 * @Function: Test function of StrTrim()
 **/
TEST(String_TestCase, StrTrim_Test)
{
    tchar testString[128] = "  \t\n\rabcdefg  \r\n";
    ASSERT_STREQ(StrTrim(testString), "abcdefg");

    tchar testString2[128] = "abcdefg  \r\n";
    ASSERT_STREQ(StrTrim(testString2), "abcdefg");

    tchar testString3[128] = "abcdefg";
    ASSERT_STREQ(StrTrim(testString3), "abcdefg");

    tchar testString4[128] = "abcd ef\r\ng";
    ASSERT_STREQ(StrTrim(testString4), "abcd ef\r\ng");
}

/**
 * @Function: Test function of Str2Argv()
 **/
TEST(String_TestCase, Str2Argv_Test)
{
    tchar testString[128] = "cmd.exe param1 param2 param3";
    tchar* argv[20] = { 0 };
    int argc = 20;
    tchar* res = Str2Argv(testString, argv, argc);
    ASSERT_EQ(argc, 4);
    ASSERT_STREQ(argv[0], "cmd.exe");
    ASSERT_STREQ(argv[1], "param1");
    ASSERT_STREQ(argv[2], "param2");
    ASSERT_STREQ(argv[3], "param3");
    free(res);

    tchar testString2[128] = "cmd.exe   param1   param2   param3   ";
    argc = 20;
    res = Str2Argv(testString2, argv, argc);
    ASSERT_EQ(argc, 4);
    ASSERT_STREQ(argv[0], "cmd.exe");
    ASSERT_STREQ(argv[1], "param1");
    ASSERT_STREQ(argv[2], "param2");
    ASSERT_STREQ(argv[3], "param3");
    free(res);

    tchar testString3[128] = "'my cmd.exe'   \"param1 with blank\"  ' param2 with blank '   param3   ";
    argc = 20;
    res = Str2Argv(testString3, argv, argc);
    ASSERT_EQ(argc, 4);
    ASSERT_STREQ(argv[0], "'my cmd.exe'");
    ASSERT_STREQ(argv[1], "\"param1 with blank\"");
    ASSERT_STREQ(argv[2], "' param2 with blank '");
    ASSERT_STREQ(argv[3], "param3");
    free(res);
}

/**
 * @Function: Test function of GetXValue()
 **/
TEST(String_TestCase, GetXValue_Test)
{
    ASSERT_EQ(GetXValue('F'), 15);
    ASSERT_EQ(GetXValue('f'), 15);
    ASSERT_EQ(GetXValue('E'), 14);
    ASSERT_EQ(GetXValue('e'), 14);
    ASSERT_EQ(GetXValue('D'), 13);
    ASSERT_EQ(GetXValue('d'), 13);
    ASSERT_EQ(GetXValue('C'), 12);
    ASSERT_EQ(GetXValue('c'), 12);    
    ASSERT_EQ(GetXValue('B'), 11);
    ASSERT_EQ(GetXValue('b'), 11);
    ASSERT_EQ(GetXValue('A'), 10);
    ASSERT_EQ(GetXValue('a'), 10);    
    ASSERT_EQ(GetXValue('9'), 9);
    ASSERT_EQ(GetXValue('8'), 8);
    ASSERT_EQ(GetXValue('7'), 7);
    ASSERT_EQ(GetXValue('6'), 6);
    ASSERT_EQ(GetXValue('5'), 5);
    ASSERT_EQ(GetXValue('4'), 4);
    ASSERT_EQ(GetXValue('3'), 3);
    ASSERT_EQ(GetXValue('2'), 2);
    ASSERT_EQ(GetXValue('1'), 1);
    ASSERT_EQ(GetXValue('0'), 0);
    ASSERT_EQ(GetXValue('x'), -1);
    ASSERT_EQ(GetXValue(','), -1);
    ASSERT_EQ(GetXValue('$'), -1);
}

/**
 * @Function: Test function of Bin2Str()
 **/
TEST(String_TestCase, Bin2Str_Test)
{
    byte testString[] = { 0x2, 0x6, 0x11, 0x23, 0xa2, 0xA1, 0xAB, 0xAb, 0xDD, 0xDe, 0xFf, 0xFF };
    char value[128] = { 0 };
    Bin2Str(testString, value, sizeof(testString), false);
    ASSERT_STREQ(value, "02061123a2a1ababdddeffff");

    Bin2Str(testString, value, sizeof(testString), true);
    ASSERT_STREQ(value, "02061123A2A1ABABDDDEFFFF");
}

/**
 * @Function: Test function of AtoX()
 **/
TEST(String_TestCase, AtoX_Test)
{
    ASSERT_EQ(AtoX("00"), 0);
    ASSERT_EQ(AtoX("01"), 1);
    ASSERT_EQ(AtoX("2"), 2);
    ASSERT_EQ(AtoX("06"), 6);
    ASSERT_EQ(AtoX("9"), 9);
    ASSERT_EQ(AtoX("a"), 10);
    ASSERT_EQ(AtoX("0B"), 11);
    ASSERT_EQ(AtoX("0c"), 12);
    ASSERT_EQ(AtoX("D"), 13);
    ASSERT_EQ(AtoX("0E"), 14);
    ASSERT_EQ(AtoX("f"), 15);
    ASSERT_EQ(AtoX("11"), 17);
    ASSERT_EQ(AtoX("19"), 25);
    ASSERT_EQ(AtoX("1d"), 29);
    ASSERT_EQ(AtoX("1F"), 31);
    ASSERT_EQ(AtoX("41"), 65);
    ASSERT_EQ(AtoX("49"), 73);
    ASSERT_EQ(AtoX("4d"), 77);
    ASSERT_EQ(AtoX("4F"), 79);
    ASSERT_EQ(AtoX("A1"), 161);
    ASSERT_EQ(AtoX("b9"), 185);
    ASSERT_EQ(AtoX("cd"), 205);
    ASSERT_EQ(AtoX("fF"), 255);
}