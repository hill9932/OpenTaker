#include "file_system.h"
#include "gtest/gtest.h"

/**
 * @Function: Test function of IsFileExist()
 **/
TEST(FileSystem_TestCase, IsFileExist_Test)
{
    CStdString  fileName = "./noexist/FileSystemTest.txt";

    DeletePath(fileName);
    ASSERT_FALSE(IsFileExist(fileName));

    CreateFilePath(fileName);
    ASSERT_TRUE(IsFileExist(fileName));

    DeletePath(fileName);
    ASSERT_FALSE(IsFileExist(fileName));

    DeletePath("./noexist");
}

/**
 * @Function: Test function of IsDirExist()
 **/
TEST(FileSystem_TestCase, IsDirExist_Test)
{
    CStdString  dirPath = "./dir1/dir2/dir3";
    ASSERT_FALSE(IsDirExist(dirPath));

    CreateAllDir(dirPath);
    ASSERT_TRUE(IsDirExist(dirPath));

    DeletePath(dirPath);
    ASSERT_FALSE(IsDirExist(dirPath));
    ASSERT_TRUE(IsDirExist("./dir1/dir2"));
    ASSERT_TRUE(IsDirExist("./dir1"));

    RemoveDir("./dir1");
    ASSERT_FALSE(IsDirExist("./dir1/dir2"));
    ASSERT_FALSE(IsDirExist("./dir1"));
}

/**
 * @Function: Test function of GetFilePath()
 **/
TEST(FileSystem_TestCase, GetFilePath_Test)
{
    CStdString  filePath = "./dir1/dir2/dir3/file_name.txt";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "./dir1/dir2/dir3");

    filePath = "./dir1/dir2/dir3/";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "./dir1/dir2/dir3");

    filePath = "./dir1/";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "./dir1");

    filePath = "./dir1////";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "./dir1");

    filePath = "./dir1/\\\\//";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "./dir1");

    filePath = "//";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "/");

    filePath = "\\";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "\\");

    filePath = "D:";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "");

    filePath = "D:/";
    ASSERT_STREQ(GetFilePath(filePath).c_str(), "D:");
}

/**
 * @Function: Test function of GetFileName()
 **/
TEST(FileSystem_TestCase, GetFileName_Test)
{
    CStdString  filePath = "./dir1/dir2/dir3/file_name.txt";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "file_name.txt");

    filePath = "./dir1/dir2/dir3/";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");

    filePath = "./dir1/";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");

    filePath = "./dir1////";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");

    filePath = "./dir1/\\\\//";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");

    filePath = "//";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");

    filePath = "\\";
    ASSERT_STREQ(GetFileName(filePath).c_str(), "");
}
