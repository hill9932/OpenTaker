#include "lua_.h"
#include "gtest/gtest.h"

class CLuaTest : public ::testing::Test
{
public:

protected:
    CLua    m_lua;
};

TEST_F(CLuaTest, LoadConfig)
{
    CStdString  config;
    config = "name      = \"hill\"\r\n"
             "age       = 23\r\n"
             "married   = true\r\n"
             "salary    = 50432.34\r\n"
             "company   = {'adobe', 'flukenetworks', 'cisco'}\r\n"
             "skill     = {cplus='good', python='excellent', c=100, algorithm='excellent', pass=true}\r\n";
    ASSERT_EQ(m_lua.doString(config), 0);

    CStdString  value;
    bool        bValue;
    double      dValue;

    //
    // test the string value
    //
    ASSERT_TRUE(m_lua.loadString("name", value));
    ASSERT_TRUE(value == "hill");

    ASSERT_FALSE(m_lua.loadString("novalue", value));
    ASSERT_TRUE(m_lua.loadString("age", value));
    ASSERT_TRUE(value == "23");

    //
    // test the bool value
    // 
    ASSERT_FALSE(m_lua.loadString("married", value));
    ASSERT_TRUE(m_lua.loadBoolean("married", bValue));
    ASSERT_TRUE(bValue);

    //
    // test the number value
    //
    ASSERT_TRUE(m_lua.loadString("salary", value));
    ASSERT_TRUE(value == "50432.34");
    ASSERT_TRUE(m_lua.loadDouble("salary", dValue));
    ASSERT_TRUE(dValue == 50432.34);

    //
    // test the array value
    //
    vector<CStdString>  arrValue;
    ASSERT_TRUE(m_lua.loadArray_s("company", arrValue));
    ASSERT_TRUE(arrValue[0] == "adobe");
    ASSERT_TRUE(arrValue[1] == "flukenetworks");
    ASSERT_TRUE(arrValue[2] == "cisco");
    ASSERT_FALSE(m_lua.loadArray_s("NOcompany", arrValue));

    //
    // test the table value
    //
    LuaObject   objValue;
    ASSERT_FALSE(m_lua.loadObject("NObject", objValue));
    ASSERT_TRUE(m_lua.loadObject("skill", objValue));
    ASSERT_TRUE(objValue["cplus"] == "good");
    ASSERT_TRUE(objValue["python"] == "excellent");
    ASSERT_TRUE(objValue["pass"] == "true");
    ASSERT_TRUE(objValue["c"] == "100");
    ASSERT_TRUE(objValue["algorithm"] == "excellent");
    ASSERT_FALSE(m_lua.loadObject("NObject", objValue));
}

TEST_F(CLuaTest, doFunc)
{
    CStdString  chunk;

}