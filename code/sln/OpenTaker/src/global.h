#ifndef __HILUO_GLOBAL_INCLUDE_H__
#define __HILUO_GLOBAL_INCLUDE_H__

#include "common.h"
#include "capture_file.h"
using namespace LiangZhu;

enum DebugMode_e
{
    DEBUG_NONE,
    DEBUG_BLOCK,
    DEBUG_PACKET
};

enum WriteMode_e
{
    WRITE_SEQUENTIAL = 1,
    WRITE_INTERLEAVED
};

enum IoMode_e
{
    IOMODE_NONE,
    IOMODE_CAPTURE_THREAD = 1,
    IOMODE_IO_THREAD
};

//
// config about engine
//
struct EngineConf_t
{
    CStdString  dbPath;         // the path to store the packet db files
    CStdString  pcapFile;
    bool        isSecAlign;
    bool        enableParsePacket;
    bool        enableS2Disk;
    bool        enableInsertDB;

    IoMode_e    ioMode;

    u_int32     blockMemSize;   // the size of share memory to cache packets data
    u_int32     captureDuration;// the time to capture
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
    u_int64 fileSize;       // default size is 512 MB
    u_int32 fileCount;      // total file count of the storage
    u_int32 sliceSize;      // the size of packet to store
    u_int32 dirLevel;
    u_int32 ioCount;
    bool    renameFile;

    vector<TargetConf_t>  fileDir;
    WriteMode_e         writeMode;
    FileType_e          fileType;
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
    bool                stopWhenWrap;
    FileStatus_e        fileStatus;
    vector<TimeSegment> lockTime;
};

struct EnvConfig_t
{
    DebugMode_e     debugMode;
    CStdString      configFile;

    EngineConf_t    engine;
    StorageConf_t   storage;
    CaptureConf_t   capture;    
    
    int             justTestCapture;
};

const int g_filesPerDir = 128;  // max file count in a directory

bool SetupEnviroment(int _argc, char** _argv, const char* _projectName);

/**
 * When multiple target config, and write mode could be sequential or interleaved
 * these function used to adjust the file info according to the write mode
 **/
CStdString GetFileSubDirByIndex(u_int32 _index, u_int32 _dirLevel);
CStdString GetFilePathByIndex(u_int32 _index, u_int32 _dirLevel);
int AdjustFileIndex(u_int32 _target, int& _fileIndex);
int GetMaxFileIndex(u_int32 _target);
int GetTargetDBIndex(u_int32 _target);
int GetTargetByFileIndex(int& _index);
int GetRecordDBCount();


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
    bool loadConfig();

    void enable(bool _enable) { m_enable = _enable; }
    bool enable() { return m_enable; }

private:
    Enviroment();
    ~Enviroment();

public:
    EnvConfig_t     m_config;
    TimeSegment     m_capTimeRange;

private:
    bool    m_enable;
};

extern Enviroment* g_env;

/**
* @Function: System signal handler
**/
#ifdef WIN32
BOOL WINAPI HandlerRoutine(DWORD _sig);
#else
void HandlerRoutine(int _sig);
#endif


#endif
