#include "common.h"
#include "version.h"
#include "system_.h"

RM_LOG_DEFINE(PROJECT_NAME);

using namespace LiangZhu;

void ShowConfig()
{
    CStdString version = "RELEASE  ";
#if defined(DEBUG) || defined(_DEBUG)
    version = "DEBUG  ";
#endif
    version += HILUO_PACKAGE_VERSION;

    RM_LOG_INFO_S("Success to initialize the environment (" << version << ", " << \
                   GetProgramBits() << " bit, pid: " << GetCurrentPID() << ").");
 
}

int main(int _argc, char** _argv)
{
    ShowConfig();

    RM_LOG_INFO("I'm going to exit.");
    return 0;
}