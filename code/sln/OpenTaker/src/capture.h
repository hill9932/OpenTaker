/**
 * @Function:
 *  This file defines the class to use pcap to capture packets.
 * @Memo:
 *  Created by hill, 4/11/2014
 **/
#ifndef __HILUO_CAPTURE_INCLUDE_H__
#define __HILUO_CAPTURE_INCLUDE_H__

#include "common.h"
#include "structure.h"
#include "device.h"
#include "capture_file.h"
#include "sqlite_.h"
#include "mutex.h"
#include "libpcap/pcap.h"

#include <fstream>

struct Packet_t
{
    struct  packet_header_t pkt_header;
    const   u_char *pkt_data;
};

struct RMonStat
{
    uint64_t total_pkts;        // total received packets on this port
    uint64_t total_bytes;       // total received bytes
    uint64_t err_pkts;
    uint64_t drop_pkts;
    uint64_t brdcst_pkts;       // broadcast pkts
    uint64_t mcst_pkts;         // multicast (but not broadcast) pkts

    uint64_t pkts_lt_64;        // count of pkts of size < 64B
    uint64_t pkts_eq_64;        // size == 64B
    uint64_t pkts_65_127;       // size 65 to 127 bytes
    uint64_t pkts_128_255;
    uint64_t pkts_256_511;
    uint64_t pkts_512_1023;
    uint64_t pkts_1024_1518;
    uint64_t pkts_1519_2047;
    uint64_t pkts_2048_4095;
    uint64_t pkts_4096_8191;
    uint64_t pkts_8192_9018;
    uint64_t pkts_9019_9198;
    uint64_t pkts_ge_9199;
    uint64_t pkts_oversize;     // ANIC has an architectural max at ~ 14KB
};

/**
 * @Function: Keep the statistic of every NIC
 **/
struct Statistic_t
{
    time_t  recordTime;

    int     index;
    int     port;
    PortStatus_e    status;

    u_int64 pktRecvCount;
    u_int64 pktSaveCount;
    u_int64 bytesRecvCount;
    u_int64 bytesSaveCount;
    u_int64 pktSeenByNIC;
    u_int64 bytesSeenByNIC;
    u_int64 pktDropCount;
    u_int64 pktFilterCount;
    u_int64 pktErrCount;
    float   utilization;

    RMonStat    rmonStat;
};

struct CaptureConfig_t
{

};

enum CaptureState_e
{
    CAPTURE_ERROR       = 1 << 0,
    CAPTURE_STOPPED     = 1 << 1,
    CAPTURE_STOPPING    = 1 << 2,
    CAPTURE_STARTED     = 1 << 3,
    CAPTURE_STARTING    = 1 << 4
};

enum Pred_e
{
    PredNone = 0,
    PredAnd,
    PredOr
};

int ParseBPFilter(Filter_t& _filter, const char* _filterString, int _port);
CStdString GenQuerySql(Filter_t _bpfFilter);
typedef int(*RecordFileFunc)(FileInfo_t*, void* _context);  // define the function to handle queried file

#define FILE_DB_NAME        "OpenTaker.db"
#define FILE_STATUS_TABLE   "FileStatusInfo"        // the table name to store all the file information, created in farewell.db
#define FILE_PACKET_TABLE   "FilePacketInfo"        // the table name to store packets meta information, created in every file db
#define FILE_BLOCK_TABLE    "FileBlockInfo"         // the table name to store block information

#define MAX_PORT_NUM        4
#define MAX_CAPTURE_THREAD  4

class CNetCapture
{
public:
    CNetCapture();
    virtual ~CNetCapture();

    int  openDevice(int _index);
    DeviceST& getDevice() { return m_device; }
    u_int64 getCapability();

    /**
     * @Function: Open the specified device to ready to capture
     **/
    virtual int  closeDevice() = 0;
    virtual bool isOpen() = 0;

    /**
    * @Function: Filter the packets to capture
    **/
    int  setCaptureFilter(const char* _filter, int _port);

    /**
     * @Function: Start to capture packets from NIC
     *  Should call openDevice() before start capture.
     * @Memo:
     *  When no packet captured, will timeout which is set at pcap_open_live()
     **/
    virtual int  startCapture(CaptureConfig_t& _config) = 0;
    virtual int  stopCapture() = 0;
    virtual int  setSlice(int _sliceCount, int _port) { return -1; };

    /**
     * @Function: Get the statistic of device such as captured bytes
     * @Param _pcapStat [Out]: keep the values of specified NIC
     * @Param _index: the index of NIC to query
     **/
    virtual int  getStatistic(Statistic_t& _pcapStat) = 0;

    /**
     * @Function: Some NIC support multiple ports, so provide a vector to keep separate stats
     **/
    virtual int  getStatistic(Statistic_t& _pcapStat, int _port) = 0;

    int  showLocalNICs();
    int  getLocalNICs(vector<DeviceST>& _NICs);
    int  getDevIndex()  { return m_devIndex; }
    void lockDevice()   { m_deviceLock.lock(); }
    void unlockDevice() { m_deviceLock.unlock(); }

    CaptureState_e getCaptureState(int _captureId) { return m_captureState[_captureId]; }
    CaptureState_e getCaptureState()
    {
        for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
            if (CAPTURE_STARTED == m_captureState[i])   
                return CAPTURE_STARTED;

        return CAPTURE_STOPPED;
    }

    DEFINE_EXCEPTION(DB);
    DEFINE_EXCEPTION(File);

protected:
    virtual ICaptureFile* createCaptureFile() = 0;
    virtual int  setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter) = 0;
    virtual bool openDevice(int _index, const char* _devName) = 0;

    virtual bool init(bool _createFile = true);
    virtual int  createExtTables() { return 0; }

    /**
    * @Function: Determine the next file to store packets.
    **/
    virtual int nextFile(bool _next = true);

    /**
     * @Function: Update the timestamp etc, in the file name
     **/
    bool updateFileName();
    CStdString getFilePath(u_int32 _index);
    CStdString getFilterString(const Filter_t& _filterItem);
    int padding(DataBlock_t* _block, int _padSize);

private:
    CStdString getNextFileName();

    /**
     * @Function: create the basic tables
     **/
    int createTables();
    int createDB(int _index, CStdString& _dbPath);

    /**
     * @Function: Sqlite will call this back when "select" some records
     **/
    static int
    GetRecord_Callback(void* _context, int _argc, char** _argv, char** _azColName);
    
protected:
    static vector<DeviceST> m_localNICs;        // keep the available NICs
    int             m_devIndex;                 // the opened device index
    DeviceST        m_device;

    CStdString      m_statTableName;           // every NIC needs a table to store the statistic every 1 sec

    u_int64         m_pktRecvCount[MAX_CAPTURE_THREAD];      // keep the count of received packets count
    u_int64         m_byteRecvCount[MAX_CAPTURE_THREAD];     // keep the count of received bytes count

    int*            m_nextFileIndex;            // the index of next file to store packets
    ICaptureFile*   m_pcapFile;                 // the file to store packets
    FileInfo_t*     m_fileInfo;
    u_int32         m_tickCount;                // count the tick number in 1 sec unit
    u_int64         m_capability;
    u_int64         m_lastPktTimestamp;

    LiangZhu::Mutex m_logMutex;                 // multiple thread may use the following log
    LiangZhu::Mutex m_deviceLock;
    ofstream        m_fsCaptureBlock;
    ofstream        m_fsCapturePacket[MAX_CAPTURE_THREAD];

    int             m_curTarget;                // when multiple target exist, specify the current target
    CaptureState_e  m_captureState[MAX_CAPTURE_THREAD];     // keep the capture state
    int             m_curDB;                    // the index to m_dbs, at most 10 target now

private:
    LiangZhu::CSqlLiteDB    m_dbs[10];                  // open the farewell.db to store file information
    bool                    m_createFile;
};

#endif
