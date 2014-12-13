#include "global.h"
#include "file_system.h"
#include "popt/popt.h"

/**
* @Function: System signal handler
**/
#ifdef WIN32
BOOL WINAPI HandlerRoutine(DWORD _sig)
#else
void HandlerRoutine(int _sig)
#endif
{
    RM_LOG_INFO("Receiving exit instruction " << _sig);
    if (g_env)   g_env->enable(false);
    SleepSec(5);

#ifdef WIN32
    return TRUE;
#endif
}

bool SetupEnviroment(int _argc, char** _argv, const char* _projectName)
{
    g_env = Enviroment::GetInstance();
    bool r = g_env->init(_argc, _argv, _projectName);

#ifdef WIN32
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
#else
    signal(SIGINT, HandlerRoutine);
    signal(SIGTERM, HandlerRoutine);
    signal(SIGSTOP, HandlerRoutine);
#endif

    return r;
}

Enviroment* g_env = NULL;
Enviroment::Enviroment()
{
    m_config.configFile = LiangZhu::GetAppDir() + "config/open_take.lua";
    m_config.engine.pcapFile = LiangZhu::GetAppDir() + "validation.pcap";
}

Enviroment::~Enviroment()
{
}

bool Enviroment::init(int _argc, char** _argv, const char* _projectName)
{
    bool ret = false;
    ret = InitLog(LiangZhu::GetAppDir() + "config/log4cplus.properties", _projectName);
    if (!ret)   return false;

    char* pcapFile = NULL;
    char* confFileName = NULL;
    struct poptOption table[] =
    {
        { "config-file",
        '\0',
        POPT_ARG_STRING,
        &confFileName,
        1,
        "specify config file name.",
        NULL},

        { "pcap-file",
        '\0',
        POPT_ARG_STRING,
        &pcapFile,
        1,
        "specify the pcap file to open",
        NULL },

        { NULL, 0, 0, NULL, 0 }
    };
    poptContext context = poptGetContext(NULL, _argc, const_cast<const char**>(_argv), table, 0);

    int r = 0;
    while ((r = poptGetNextOpt(context)) >= 0);
    if (r < -1)
    {
        poptPrintHelp(context, stderr, 0);
        cerr << "Has invalid command options.";
        poptFreeContext(context);
        return false;
    }
    poptFreeContext(context);

    if (confFileName)   m_config.configFile = confFileName;
    if (pcapFile)       m_config.engine.pcapFile = pcapFile;

    return ret;
}

bool Enviroment::loadConfig()
{
    return true;
}

bool CreateSubDirs(CStdString& _path, int _startIndex, int _stopIndex, int& _itemLeft, int _level, bool _createFile = false)
{
    CStdString filePath;
    int stop = MyMin(_stopIndex, _itemLeft);
    for (int i = _startIndex; i < stop; ++i)
    {
        if (_level > 0)
        {
            filePath.Format("%s/%04d", _path.c_str(), i);
            CreateAllDir(filePath);
            CreateSubDirs(filePath, 0, stop, _itemLeft, _level - 1, _createFile);
        }
        else if (_createFile)
        {
            filePath.Format("%s/%04d-0-0-0-0-0.pcap", _path.c_str(), i);
            AllocateFile(filePath, g_env->m_config.storage.fileSize);
        }

        if (_itemLeft <= 0) break;
    }

    if (_level == 0)    _itemLeft -= stop;
    return true;
}

CStdString GetFileSubDirByIndex(u_int32 _index, u_int32 _dirLevel)
{
    assert(_index < g_env->m_config.storage.fileCount);
    if (_index >= g_env->m_config.storage.fileCount)    return "";

    CStdString subDir;
    CStdString tmp;
    int index = _index;
    for (u_int32 i = 0; i < _dirLevel; ++i)
    {
        int levelCount = (int)pow(g_filesPerDir, _dirLevel - i);
        tmp.Format("%04d/", index / levelCount);
        subDir += tmp;
        index %= levelCount;
    }

    return subDir;
}

CStdString GetFilePathByIndex(u_int32 _index, u_int32 _dirLevel)
{
    int target = 0;
    u_int32 count = g_env->m_config.storage.fileDir[target].fileCount;

    while (count < _index)
    {
        count += g_env->m_config.storage.fileDir[++target].fileCount;
    }

    if (g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL)
        _index -= g_env->m_config.storage.fileDir[target].fileIndexOffset;

    CStdString path;
    path.Format("%s/%s", g_env->m_config.storage.fileDir[target].target.c_str(),
        GetFileSubDirByIndex(_index, _dirLevel).c_str());
    return path;
}

int AdjustFileIndex(u_int32 _target, int& _fileIndex)
{
    assert(_target < g_env->m_config.storage.fileDir.size());
    if (_target >= g_env->m_config.storage.fileDir.size())  return -1;

    if (g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL)
        _fileIndex -= g_env->m_config.storage.fileDir[_target].fileIndexOffset;

    return 0;
}

int GetMaxFileIndex(u_int32 _target)
{
    assert(_target < g_env->m_config.storage.fileDir.size());
    if (_target >= g_env->m_config.storage.fileDir.size())  return -1;

    return g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL ?
        g_env->m_config.storage.fileCount :
        g_env->m_config.storage.fileDir[_target].fileCount;
}

int GetRecordDBCount()
{
    return g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL ?
        1 : g_env->m_config.storage.fileDir.size();
}

int GetTargetDBIndex(u_int32 _target)
{
    return g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL ? 0 : _target;
}

int GetTargetByFileIndex(int& _index)
{
    int target = 0;
    int count = g_env->m_config.storage.fileDir[target].fileCount;

    while (count <= _index)
    {
        count += g_env->m_config.storage.fileDir[++target].fileCount;
    }

    return target;
}
