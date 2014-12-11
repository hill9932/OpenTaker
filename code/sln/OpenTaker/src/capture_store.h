#ifndef __HILUO_CAPTURE_STORE_INCLUDE_H__
#define __HILUO_CAPTURE_STORE_INCLUDE_H__

#include "block_capture.h"
#include "packet_ring_class.h"

using namespace LiangZhu;

class CBlockCaptureImpl : public CBlockCapture, public ISingleton<CBlockCaptureImpl>
{
public:
    /**
    * @Function: these function are used just like CBlockCapture
    **/
    virtual int  getStatistic(Statistic_t& _pcapStat);
    virtual int  getStatistic(Statistic_t& _pcapStat, int _port);

    virtual int  startCapture();
    virtual int  stopCapture();

    u_int64 getCapacity();
    u_int64 getTotalPacketCount();
    u_int64 getFirstPacketTime();
    u_int64 getLastPacketTime();
    u_int64 getFirstNonLockedPktTime();

    double  getDiskUsage();
    double  getLockedDisk();
    /**
     * Return the count of storage files that successfully locked
     *  -1   cannot lock packet, otherwise more than 80% stroage space will
     *       be locked
     *  -2   SQLITE op failed
     */
    int     lockFile(u_int64 _firstTime, u_int64 _lastTime);
    /**
     * Return the count of storage files that successfully unlocked
     *  -2 means SQLITE op failed
     */
    int     unlockFile(u_int64 _firstTime, u_int64 _lastTime);
    /**
     * Return value:
     *   0:  deletion op is succeed
     *  -1:  packets are not ready to be deleted
     *  -2:  SQLITE op failed
     */
    int     deletePacket(u_int64 _firstTime, u_int64 _lastTime);

    CStdString getCurrentFile();

    /**
    * @Function: these functions are used by CBlockCapture objects
    **/
    DataBlock_t* requestBlock(u_int32 _captureId);
    PacketMeta_t *requestMeta4PktBlk(u_int32 _captureId, DataBlockHeader_t *dataBlk);
    u_int32 getCurMetaBlkPos(u_int32 captureId);
    void finalMetaBlk(u_int32 _captureId);
    void sendStopSignal();

    int  releaseBlock(DataBlock_t* _block);
    int  flushFile(u_int64 _tickCount);
    int  flushDB();

    inline int getFinishedRequest(vector<PPER_FILEIO_INFO_t>& _request)
    {
        return m_filePool.getFinishedRequest(_request);
    }

    inline int finalizeIo(PPER_FILEIO_INFO_t _ioContext)
    {
        int z = OnDataWritten(NULL, _ioContext, 0);
        m_filePool.release(_ioContext);
        return z;
    }

    void stopFilePool()
    {
        m_filePool.stop();
    }

    int  blockArrived(DataBlock_t* _block, int _blockCount = 1);
    void regObj(CNetCapture* _obj);
    void unregObj(CNetCapture* _obj);
    void join();

    /**
    * @Function: these function are used by thrift service
    **/
    int startCapture(int _index, const tchar* _filter = NULL);
    int startCapture(int _index, CaptureConfig_t& _filter);

    int stopCapture(int _index);
    int setFilter(int _index, int _port, const tchar* _filter);
    int getStatistic(Statistic_t& _pcapStat, int _index, int _port);

    DeviceST* getDevice(int _index);
    int getCapObjCount() { return m_capObjMap.size(); }

    void processMetaBlock(PktMetaBlk_t *_metaBlk);

    DEFINE_EXCEPTION(Init);

protected:
    /**
    * @Function: called in the process thread
    *  its main job is to get the packets in memory and call the agent to do basic analysis
    **/
    static int DoTheProcess(void* _obj);

    /**
    * @Function: called in the store thread
    *  its main job is to store the block data into disk and release the share memory
    **/
    static int DoTheStore(void* _obj);

    /**
    * @Function: add the NIC statistic to database.
    **/
    static int DoTheDBStats(void* _obj);

    static int DoTheTimer(void* _obj);

    static int GetRecord_Callback(void* _context, int _argc, char** _argv, char** _szColName);

    /**
    * @Function: called in the FlowGen thread
    *  its main job is to get the block from the list and parse every packets
    *  in the block to get the basic information.
    **/
    virtual ICaptureFile* createCaptureFile();
    virtual int nextFile(bool _next);
    virtual int prepareResource(CaptureConfig_t& _config);
    virtual bool openDevice_(int _index, const char* _devName) { return false;   }
    virtual bool closeDevice_() { return false; }

    int  startStoreImpl();
    bool isOpen()
    {
        return m_curOutRes >= 0 && m_capObjMap.size() > 0;
    }

    CNetCapture* getCapObj(int _index);

    /**
    * @Function: Callback function after data written to file
    **/
    static int
    OnDataWritten(void* _handler, PPER_FILEIO_INFO_t _ioContext, DWORD _bytesCount);

    static int
    OnDataWrittenError(void* _handler, PPER_FILEIO_INFO_t _ioContext, int _err);

    static int
    ClearFile(FileInfo_t* _fileInfo, void* _context);

    void releaseBlockResource(DataBlock_t* _block);
    void releasePktsInBlk(PktMetaBlk_t *_metaBlk, bool _writeSucceed);
    void releasePacketResource(PacketType_e type, PacketMeta_t* _metaInfo);
    int  updateStatistic(time_t _now = 0, const char* _step = "");
    int  queryRecordDB(const tchar* _sql, vector<u_int64>& _result);
    int  execOnRecordDB(const tchar* _cond, RecordFileFunc _func, void* _context);
    int  getFileInfoList(u_int64 _firstTime, u_int64 _lastTime, vector<FileInfo_t> &files);

    CProducerRingPtr preparePacketRing();

private:
    friend class ISingleton<CBlockCaptureImpl>;

    CBlockCaptureImpl();
    ~CBlockCaptureImpl();

    int createFileDB(bool _override = true);
    void constructExtIndices();
    int addPacketRecord(PacketMeta_t* _metaInfo, int _curOutRes);
    int addBlockRecord(DataBlock_t* _block);
    int saveBlock(DataBlock_t* _block);
    int saveBlock(PACKET_BLOCK_LIST& _blockList);
    int finalizeBlock(DataBlock_t* _block, bool _ok);
    int finalize();
    int loadResource();
    void loadAppWars();
    int prepareNextFile();

    int64 saveData(DataBlock_t* _dataBlock, u_int32 _flag = 0);

    struct OutRes_t
    {
        ICaptureFile*   pcapFile;
#ifdef USE_SQLITE
        CSqlLiteDB      blkInfoDB;           // used to keep packet's meta information
#else
        CBlockFile      blkInfoDB;
#endif
        CBlockFile      pktMetaDB;
    };

    void onTimer();
    void switchTarget();

    void recyclePktMeta(DataBlock_t *_block, bool _writeSucceed);

    u_int64 getLockedFileCount();

private:
    CFixBufferPool<DataBlock_t> m_bufPool[MAX_CAPTURE_THREAD];    // every port thread own its block
    Mutex                   m_blockListMutex;
    PACKET_BLOCK_LIST       m_blockList;        // the captured packet block list and need to process->engine->store
    CSimpleThread*          m_processThread;    // a thread to call PacketRing to do basic analysis
    CSimpleThread*          m_storeThread;      // a thread to do store job
    CSimpleThread*          m_DBStatsThread;    // a thread to insert NIC stats into db
    CSimpleThread*          m_timerThread;      // a thread to set the timer

    CFilePool               m_filePool;

    process_t               m_appWarProcess;
    Mutex                   m_capMapMutex;
    map<int, CNetCapture*>  m_capObjMap;        // keep all the capture objects
    vector<PacketMeta_t>    m_retryRecord;      // keep the insert failed record

    OutRes_t                m_outRes[20];
    int*                    m_curOutRes;        // the index of m_outRes this block belong to
    int                     m_outResCount;      // total number of m_outRes used
    int                     m_resPerTarget;

    vector<int>             m_extIndexSize;     // the total space cost by extension fields.

    Mutex                   m_statsListMutex;
    list<Statistic_t>       m_NICStatsList;     // every 1 sec will generator a record, checked by m_tickTime tick

    boost::asio::io_service m_TimerIO;
    boost::asio::deadline_timer m_Timer;

    ofstream                m_fsSaveBlock;      // log files
    ofstream                m_fsCostBlock;
    ofstream                m_fsSavePacket;

    u_int64                 m_launchWriteCount;
    u_int64                 m_finishWriteCount;

public:
    CProducerRingPtr        m_packetRing;
    ModuleInfo_t            m_modInfo;
    CaptureState_e          m_captureState;

    u_int64                 m_savePktCount;
    u_int64                 m_saveByteCount;
};




#endif
