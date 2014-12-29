#include "global.h"
#include "file_system.h"
#include "packet_ring.h"
#include "packet_ray.h"
#include "string_.h"
#include "system_.h"
#include "code_convert.h"
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

    g_env->enable(true);
    return r;
}

Enviroment* g_env = NULL;
Enviroment::Enviroment()
{    
    m_enable = false;
    m_config.configFile             = LiangZhu::GetAppDir() + "config/open_taker.lua";
    m_config.capture.captureMode    = 0;
    m_config.capture.stopWhenWrap   = false;
    m_config.capture.pcapFile       = LiangZhu::GetAppDir() + "validation.pcap";

    m_config.storage.fileSize       = 512 * ONE_MB;
    m_config.storage.sliceSize      = 64;
    m_config.storage.fileType       = FILE_PCAP;

    m_config.engine.dbPath          = LiangZhu::GetAppDir() + "filedb";
    m_config.engine.blockMemSize    = 10 * ONE_MB;
    m_config.engine.debugMode       = DEBUG_BLOCK;
    m_config.engine.enableInsertDB  = false;
    m_config.engine.enableS2Disk    = true;
    m_config.engine.enableParsePacket = true;
    m_config.engine.ioCount         = 16;
    m_config.engine.ioMode          = IOMODE_IO_THREAD;
}

Enviroment::~Enviroment()
{
}

bool Enviroment::init(int _argc, char** _argv, const char* _projectName)
{
    bool ret = false;
    ret = InitLog(LiangZhu::GetAppDir() + "config/log4cplus.properties", _projectName);
    if (!ret)   return false;

    //
    // set the share lib use my logger
    //
    SetRingLogger(g_logger);
    SetRayLogger(g_logger);

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
    if (pcapFile)       m_config.capture.pcapFile = pcapFile;

    return loadConfig();
}

#define KEY_STORAGE                 "storage"
#define KEY_STORAGE_PATH            "path"
#define KEY_STORAGE_FILESIZE        "file_size"
#define KEY_STORAGE_SLICESIZE       "slice_size"
#define KEY_STORAGE_WRITE_MODE      "write_mode"
#define KEY_STORAGE_IO_COUNT        "io_count"

#define KEY_ENGINE                  "engine"
#define KEY_ENGINE_BLOCKSIZE        "block_size"
#define KEY_ENGINE_BLOCKMEM_SIZE    "block_share_memory_size"
#define KEY_ENGINE_METAMEM_SIZE     "meta_share_memory_size"
#define KEY_ENGINE_ENABLE_N2DISK    "enable_n2disk"
#define KEY_ENGINE_PARSE_PACKET     "enable_parse_packet"
#define KEY_ENGINE_ENABLE_N2DB      "enable_insert_db"
#define KEY_ENGINE_DB_PATH          "db_path"
#define KEY_ENGINE_DEBUG_MODE       "debug_mode"    // 1 - record block information 2 - record packet information, default is 0
#define KEY_ENGINE_IO_MODE          "io_mode"       // 1 - own thread to recycle finished io 2 - capture thread to recycle


bool Enviroment::loadConfig()
{
    LuaObject   confObj;
    if (0 != m_lua.doFile(m_config.configFile))  return false;
    m_lua.loadObject(KEY_STORAGE, confObj);

    if (confObj.size() == 0)  return false;
    if (confObj.count(KEY_STORAGE_PATH) == 0)
    {
        RM_LOG_ERROR("Please config the storage path.");
        return false;
    }

    //
    // check path
    //
    CStdString value = confObj[KEY_STORAGE_PATH];
    if (value.empty())    return false;
    string ansiValue;
    Utf82Ansi(value.data(), value.size(), ansiValue);
    if (!ansiValue.empty())
    {
        //
        // multiple paths are split with ";"
        //
        vector<CStdString> paths;
        StrSplit(ansiValue.c_str(), TEXT(";"), paths);
        if (paths.size() == 0)  return false;

        if (confObj.count(KEY_STORAGE_FILESIZE))
        {
            m_config.storage.fileSize = MyMax(atoi_t(confObj[KEY_STORAGE_FILESIZE]), 2);
            m_config.storage.fileSize *= ONE_MB;
        }

        if (confObj.count(KEY_STORAGE_SLICESIZE))
            m_config.storage.sliceSize = atoi_t(confObj[KEY_STORAGE_SLICESIZE]);

        //
        // path and size are split with "|"
        //
        for (unsigned int i = 0; i < paths.size(); ++i)
        {
            CStdString& value = paths[i];
            TargetConf_t conf;
            vector<CStdString> vv;
            StrSplit(value.c_str(), TEXT("|"), vv);
            if (vv.size() == 0)   continue;

            conf.target = vv[0].Trim();    // target path
            if (vv.size() == 2 && atoi_t(vv[1]) > 0)     // target capacity
                conf.capacity = atoi_t(vv[1]) * ONE_GB;
            else if (atoi_t(vv[1]) == -1)
            {
                conf.capacity = GetAvailableSpace(conf.target);
                if (conf.capacity == 0)  conf.capacity = ONE_GB;
                else conf.capacity *= ONE_GB;
            }
            else
            {
                cout << "Use config a invalid disk storage size, we will set it as 1 GB." << endl;
                conf.capacity = ONE_GB;
            }

            conf.curFileIndex = 0;
            conf.fileIndexOffset = m_config.storage.fileCount;
            conf.fileCount = conf.capacity / m_config.storage.fileSize;
            conf.firstFileIndex = conf.curFileIndex = 0;

            conf.outResIndex = -1;
            bzero(&conf.fileInfo, sizeof(FileInfo_t));
            m_config.storage.fileDir.push_back(conf);
            m_config.storage.fileCount += conf.fileCount;

            if (!CreateAllDir(conf.target))
            {
                cout << "Create all directories " << conf.target << " failed, "
                     << GetLastSysErrorMessage(GetLastSysError()).c_str() << endl;
                return false;
            }
        }
    }

    //
    // configure of "engine"
    //
    confObj.clear();
    m_lua.loadObject(KEY_ENGINE, confObj);

    // block share memory size
    if (confObj.count(KEY_ENGINE_BLOCKMEM_SIZE))
    {
        m_config.engine.blockMemSize = MyMax(atoi_t(confObj[KEY_ENGINE_BLOCKMEM_SIZE]), 10);
        m_config.engine.blockMemSize *= ONE_MB;
    }

    if (confObj.count(KEY_ENGINE_ENABLE_N2DISK))
        m_config.engine.enableS2Disk = stricmp("true", confObj[KEY_ENGINE_ENABLE_N2DISK]) == 0;

    if (confObj.count(KEY_ENGINE_PARSE_PACKET))
        m_config.engine.enableParsePacket = stricmp("true", confObj[KEY_ENGINE_PARSE_PACKET]) == 0;

    if (confObj.count(KEY_ENGINE_ENABLE_N2DB))
        m_config.engine.enableInsertDB = stricmp("true", confObj[KEY_ENGINE_ENABLE_N2DB]) == 0;

    if (confObj.count(KEY_ENGINE_DB_PATH))
        m_config.engine.dbPath = confObj[KEY_ENGINE_DB_PATH].Trim();

    if (confObj.count(KEY_ENGINE_DEBUG_MODE))
        m_config.engine.debugMode = (DebugMode_e)atoi_t(confObj[KEY_ENGINE_DEBUG_MODE]);

    if (confObj.count(KEY_ENGINE_IO_MODE))
        m_config.engine.ioMode = (IoMode_e)atoi_t(confObj[KEY_ENGINE_IO_MODE]);

    return createDirs();
}

bool Enviroment::createDirs()
{
    bool b = false;
    for (u_int32 i = 0; i < m_config.storage.fileDir.size(); ++i)
    {
        CStdString targetName;
        targetName.Format("%s/target%d", g_env->m_config.engine.dbPath, i);
        b = CreateAllDir(targetName);
        if (m_config.storage.writeMode == WRITE_SEQUENTIAL)
            break;
    }

    b = CreateAllDir(GetAppDir() + "./PerfTest");

    return b; 
}