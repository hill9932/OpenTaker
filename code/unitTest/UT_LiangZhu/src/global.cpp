#include "global.h"
#include "file_system.h"


Enviroment* g_env = NULL;
Enviroment::Enviroment()
{

}

Enviroment::~Enviroment()
{

}

bool Enviroment::init(int _argc, char** _argv, const char* _projectName)
{
    bool ret = false;
    ret = InitLog(LiangZhu::GetAppDir() + "config/log4cplus.properties", _projectName);

    return ret;
}
