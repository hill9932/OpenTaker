#include "common.h"
#include "system_.h"
#include "global.h"
#include "version.h"
#include "gtest/gtest.h"


RM_LOG_DEFINE(PROJECT_NAME);


void ShowConfig()
{
    CStdString version = LiangZhu::GetLibVersion();

    RM_LOG_INFO_S("Success to initialize the environment (Lib version = " << version << ", " << \
                   GetProgramBits() << " bit, pid: " << GetCurrentPID() << ").");
 
}


int main(int _argc, char** _argv)
{
    if (!SetupEnviroment(_argc, _argv, PROJECT_NAME))
        return -1;
    ShowConfig();

    testing::InitGoogleTest(&_argc, _argv);

    return RUN_ALL_TESTS();
}