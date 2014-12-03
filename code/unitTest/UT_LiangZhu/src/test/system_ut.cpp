#include "system_.h"
#include "file_system.h"
#include "gtest/gtest.h"

/**
 * @Function: Test function of GetAvailableSpace()
 **/
TEST(System_TestCase, GetAvailableSpace_Test)
{
    bool bCreate = false;
    const char* checkDir = "/share";

    if (!IsDirExist(checkDir))
    {
        CreateAllDir(checkDir);
        bCreate = true;
    }

    ASSERT_TRUE(IsDirExist(checkDir));
    u_int64 space = GetAvailableSpace(checkDir);
    ASSERT_GT(space, 1);
    
    if (bCreate)
    {
        DeletePath(checkDir);
        ASSERT_FALSE(IsDirExist(checkDir));
    }
}

/**
 * @Function: Test function of ListDir()
 **/
TEST(System_TestCase, ListDir_Test)
{
    const char* checkDir = "C:";
    vector<CStdString>  dirs;
    ListDir(checkDir, dirs);
    ASSERT_GT(dirs.size(), 1);
}