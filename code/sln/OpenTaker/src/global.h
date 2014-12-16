#ifndef __HILUO_GLOBAL_INCLUDE_H__
#define __HILUO_GLOBAL_INCLUDE_H__

#include "common.h"
#include "storage.h"
#include "capture_file.h"
#include "lua_.h"

using namespace LiangZhu;

enum DebugMode_e
{
    DEBUG_NONE,
    DEBUG_BLOCK,
    DEBUG_PACKET
};

enum IoMode_e
{
    IOMODE_NONE,
    IOMODE_CAPTURE_THREAD = 1,  // use the same thread as capture
    IOMODE_IO_THREAD            // a separate thread to get the finished IO
};

//
// config about engine
//
struct EngineConf_t
{
    CStdString  dbPath;         // the path to store the packet db files
    bool        enableParsePacket;
    bool        enableS2Disk;
    bool        enableInsertDB;

    IoMode_e    ioMode;
    DebugMode_e debugMode;

    u_int32     ioCount;
    u_int32     blockMemSize;   // the size of share memory to cache packets data
};

struct TargetConf_t
{
    CStdString  target;         // the path of storage
    u_int64     capacity;       // the capacity of storage, default is -1 which means all the space
    int         curFileIndex;   // the current file index that's been written
    int         fileIndexOffset;// file index relative to the current target when write mode is sequential
                                // used to locate the file path
    FileInfo_t  fileInfo;       // keep the current written file's information
    int         outResIndex;    // index the pending open file
    int         firstFileIndex; // the start file index
    int         fileCount;      // file count in this target
};

//
// config about storage
//
struct StorageConf_t
{
    u_int64     fileSize;       // default size is 512 MB
    u_int32     fileCount;      // total file count of the storage
    u_int32     sliceSize;      // the size of packet to store
    u_int32     dirLevel;
    bool        renameFile;
    bool        secAlign;

    WriteMode_e             writeMode;
    FileType_e              fileType;
    vector<TargetConf_t>    fileDir;
};

struct TimeSegment
{
    u_int64 startTime;
    u_int64 stopTime;

    TimeSegment()
    {
        startTime = stopTime = 0;
    }

    bool operator==(const TimeSegment& _r)
    {
        return startTime == _r.startTime && stopTime == stopTime;
    }
};

struct CaptureConf_t
{
    int                 captureMode;
    bool                stopWhenWrap;
    u_int32             captureDuration;    // the time to capture

    CStdString          pcapFile;           // the pcap file to open
    FileStatus_e        fileStatus;         // the default file status of this capture session
    vector<TimeSegment> lockTime;
};

struct EnvConfig_t
{
    CStdString      configFile;

    EngineConf_t    engine;
    StorageConf_t   storage;
    CaptureConf_t   capture;    
};

bool SetupEnviroment(int _argc, char** _argv, const char* _projectName);


/**
 * @Function:¡¡Keep the global setting for the process
 **/
class Enviroment
{
public:
    static Enviroment* GetInstance()
    {
        static Enviroment* instance = NULL;
        if (!instance)  instance = new Enviroment;
        return instance;
    }

    bool init(int _argc, char** _argv, const char* _projectName);

    void enable(bool _enable) { m_enable = _enable; }
    bool enable() { return m_enable; }

private:
    Enviroment();
    ~Enviroment();

    bool loadConfig();
    bool createDirs();

public:
    EnvConfig_t     m_config;
    TimeSegment     m_capTimeRange;

private:
    bool    m_enable;
    CLua        m_lua;
};

extern Enviroment* g_env;


#endif
