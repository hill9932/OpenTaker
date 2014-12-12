#include "capture_store.h"
#include "virtual_capture.h"
#include "pcap_capture.h"
#include "global.h"
#include "string_.h"
#include "pcap_file_fake.h"
#include "file_system.h"
#include "system_.h"
#include "net_util.h"
#include "sqlite_.h"
#include "math_.h"

#include <boost/foreach.hpp>
#include <numeric>

#define LOG_PERFORMANCE_DATA()  \
{   \
struct timeval ts;          \
    gettimeofday(&ts, NULL);    \
    TIME_INTERVAL   now;        \
    GetNowTime(now);            \
    CStdString ss;              \
    if (g_env->m_config.engine.enableParsePacket) \
    { \
        vector<u_int32> vec;   \
        m_packetRing->GetAllModulePosInfo(vec);      \
        for (unsigned int i = 0; i < vec.size(); ++i)        \
        {                           \
            char tmp[16] = { 0 };     \
            sprintf_t(tmp, "%d, ", vec[i]); \
            ss += tmp;              \
        }                           \
    } \
    capture->m_fsCostBlock << ts.tv_sec << "." << ts.tv_usec << ": " \
    << block->dataDesc.id << ", "\
    << (void*)block << ", " \
    << block->dataDesc.usedSize << ", " \
    << block->dataDesc.firstPacketTime << ", "  \
    << block->dataDesc.offset << ", " << ss << endl; \
}

int CBlockCaptureImpl::loadResource()
{
#ifdef LINUX
    char buf[4096] = { 0 };
    CStdString cmd;
    FILE* stream = NULL;
    char* z = NULL;
    int freePages = 0;

    RM_LOG_INFO("Set kernel.shmmax = 536870912");
    cmd = "sudo sysctl -w kernel.shmmax=536870912";
    stream = popen(cmd, "r");
    ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, == , NULL, return -1);

    cmd = "cat /proc/meminfo | grep HugePages_Free";
    stream = popen(cmd, "r");
    ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, == , NULL, return -1);
    z = fgets(buf, 4096, stream);
    while (z)
    {
        vector<CStdString> infos;
        StrSplit_s(buf, ": ", infos);
        if (infos.size() == 2)
        {
            freePages = atoi(infos[1].c_str());
            RM_LOG_INFO(freePages << " free huge pages available.");
        }
        else
        {
            RM_LOG_INFO("It's better to create huge pages.");
        }
        z = fgets(buf, 4096, stream);
    }
    pclose(stream);
    stream = NULL;


    //
    // mount hugetlbfs and create huge pages
    //
    if (freePages < 512)
    {
        bzero(buf, sizeof(buf));
        cmd = "sudo mkdir -p /mnt/hugetlbfs 2>&1";
        stream = popen(cmd, "r");
        ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, == , NULL, return -1);
        while (z)
        {
            if (strstr(buf, "ERROR"))
            {
                RM_LOG_ERROR("sudo mkdir -p /mnt/hugetlbfs: " << buf);
                pclose(stream);
                return -1;
            }
            z = fgets(buf, 4096, stream);
        }
        RM_LOG_INFO("Create hugepage dir @ /mnt/hugetlbfs success. ");
        pclose(stream);
        stream = NULL;

        bzero(buf, sizeof(buf));
        cmd = "sudo mount -t hugetlbfs nodev /mnt/hugetlbfs 2>&1";
        stream = popen(cmd, "r");
        ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, == , NULL, return -1);
        z = fgets(buf, 4096, stream);
        while (z)
        {
            if (strstr(buf, "ERROR"))
            {
                RM_LOG_ERROR("sudo mount -t hugetlbfs nodev /mnt/hugetlbfs: " << buf);
                pclose(stream);
                return -1;
            }
            z = fgets(buf, 4096, stream);
        }
        RM_LOG_INFO("Mount hugetlbfs @ /mnt/hugetlbfs success.");
        pclose(stream);
        stream = NULL;

        //
        // setup the number of hugepage
        //
        bzero(buf, sizeof(buf));
        cmd = "echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages 2>&1";
        stream = popen(cmd, "r");
        ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, == , NULL, return -1);
        z = fgets(buf, 4096, stream);
        while (z)
        {
            if (strstr(buf, "ERROR"))
            {
                RM_LOG_ERROR("echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages: " << buf);
                pclose(stream);
                return -1;
            }
            z = fgets(buf, 4096, stream);
        }
        RM_LOG_INFO("Setup 1024 huge pages success.");
        pclose(stream);
        stream = NULL;
    }

#endif

    return 0;
}

void CBlockCaptureImpl::loadAppWars()
{

}

CBlockCaptureImpl::CBlockCaptureImpl()
: m_filePool(g_env->m_config.engine.blockMemSize / sizeof(DataBlock_t), 1)  // create only one thread to recycle io
, m_Timer(m_TimerIO, boost::posix_time::seconds(1))
{
    m_captureState = CAPTURE_STOPPED;

    if (0 != loadResource())
    {
        RM_LOG_ERROR("Fail to load resource.");
        THROW_EXCEPTION(Init, "loadResource");
    }

    if (g_env->m_config.engine.ioMode == IOMODE_IO_THREAD)
    {
        m_filePool.setWriteCompleteCallback(OnDataWritten);
        m_filePool.setErrorCallback(OnDataWrittenError);
        m_filePool.start(NULL);
    }

    m_myImpl = this;
    m_saveByteCount = m_savePktCount = 0;

    if (!init(false))
    {
        THROW_EXCEPTION(Init, "init");
    }

    //
    // recover the pending recorder whose status is still STATUS_CAPTURE
    //
    CStdString sql;
    sql.Format("UPDATE " FILE_STATUS_TABLE " SET STATUS=%d WHERE STATUS=%d", STATUS_NORMAL, STATUS_CAPTURE);
    int z = execOnRecordDB(sql, NULL, NULL);

    if (g_env->m_config.debugMode >= DEBUG_BLOCK)
    {
        m_fsSaveBlock.open("./PerfTest/ResultBlock.txt", ios_base::out | ios_base::trunc);
        m_fsCostBlock.open("./PerfTest/CostBlock.txt", ios_base::out | ios_base::trunc);

        if (g_env->m_config.debugMode >= DEBUG_PACKET)
            m_fsSavePacket.open("./PerfTest/ResultPacket.txt", ios_base::out | ios_base::trunc);
    }

    m_curTarget = -1;
    m_curOutRes = NULL;
    m_outResCount = MyMin((u_int32)OUT_RESOUCE_COUNT, g_env->m_config.storage.fileCount);
    m_resPerTarget = m_outResCount;
    m_launchWriteCount = m_finishWriteCount = 0;
    m_outResCount *= GetRecordDBCount();

    for (unsigned int i = 0; i < sizeof(m_outRes) / sizeof(m_outRes[0]); ++i)
    {
        m_outRes[i].pcapFile = NULL;
    }

#ifndef _NO_CONSOLE_OUTPUT
    m_Timer.async_wait(boost::bind(&CBlockCaptureImpl::onTimer, this));
    m_timerThread = new CSimpleThread(DoTheTimer, this);
#endif

    m_processThread = m_storeThread;
    m_processThread = new CSimpleThread(DoTheProcess, this);
    m_storeThread = new CSimpleThread(DoTheStore, this);
}

CBlockCaptureImpl::~CBlockCaptureImpl()
{
    if (m_packetRing)
        m_packetRing->Release();
    join();

    SAFE_DELETE(m_processThread);
    SAFE_DELETE(m_storeThread);

    if (m_appWarProcess)    KillProcess(m_appWarProcess);
}

void CBlockCaptureImpl::join()
{
    m_filePool.stop();
    stopCapture();

    if (m_processThread)    m_processThread->join();
    if (m_storeThread)      m_storeThread->join();
}

int CBlockCaptureImpl::finalize()
{
    flushFile(0);
    while (m_finishWriteCount != m_launchWriteCount)    // wait until all the io are finished
    {
        SleepSec(1);
    }

    for (unsigned int i = 0; i < sizeof(m_outRes)/sizeof(m_outRes[0]); ++i)
    {
        if (m_outRes[i].pcapFile)   m_outRes[i].pcapFile->close();
        m_outRes[i].blkInfoDB.close();
        m_outRes[i].pktMetaDB.close();
    }

    for (u_int32 i = 0; i < g_env->m_config.storage.fileDir.size(); ++i)
    {
        bzero(&g_env->m_config.storage.fileDir[i].fileInfo, sizeof(FileInfo_t));
        g_env->m_config.storage.fileDir[i].outResIndex = 0;
    }

    return 0;
}

int CBlockCaptureImpl::prepareResource(CaptureConfig_t& _config)
{
    //
    // determine the stored file format
    //
    map<int, CNetCapture*>::const_iterator it = m_capObjMap.begin();
    if (it->second->getDevice().type == DEVICE_TYPE_ACCOLADE &&
        it->second->getDevice().productID >= 0x20)
    {
        g_env->m_config.storage.fileType = FILE_ACCOLADE;
        RM_LOG_INFO("Target file format will be FILE_ACCOLADE.");
    }
    else if (it->second->getDevice().type == DEVICE_TYPE_MEMORY ||
             it->second->getDevice().type == DEVICE_TYPE_NAPATECH)
    {
        g_env->m_config.storage.fileType = FILE_PV;
        RM_LOG_INFO("Target file format will be FILE_PV");
    }
    else
    {
        g_env->m_config.storage.fileType = FILE_PCAP;
        g_env->m_config.engine.isSecAlign = false;
        RM_LOG_INFO("Target file format will be FILE_PCAP.");
    }

    for (unsigned int i = 0; i < sizeof(m_outRes) / sizeof(m_outRes[0]); ++i)
    {
        if (m_outRes[i].pcapFile)   m_outRes[i].pcapFile->close();
        if (m_outRes[i].blkInfoDB.isValid())  m_outRes[i].blkInfoDB.close();

        if (i < (unsigned int)m_outResCount)
            m_outRes[i].pcapFile = createCaptureFile();
        else
            m_outRes[i].pcapFile = NULL;
    }

    for (int i = 0; i < GetRecordDBCount(); ++i)
    {
        m_curTarget = i;
        m_curDB = i;
        m_nextFileIndex = &g_env->m_config.storage.fileDir[i].curFileIndex;
        m_fileInfo = &g_env->m_config.storage.fileDir[i].fileInfo;
        m_curOutRes = &g_env->m_config.storage.fileDir[i].outResIndex;
        if (0 != nextFile(true))
        {
            RM_LOG_INFO("Fail to open the next file.");
            return -1;
        }

        RM_LOG_INFO("Target" << i <<" first file = " << m_pcapFile->getFileName());
    }

    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
        m_bufPool[i].stop(false);

    g_env->m_capTimeRange.startTime = (u_int64)-1;
    g_env->m_capTimeRange.stopTime = 0;
    g_env->m_config.capture.fileStatus = STATUS_NORMAL;

    return 0;
}

/**
* @Function: when no need to parse packet, it will responsible to save the block to disk.
*  otherwise, doTheStore will do the job.
**/

int CBlockCaptureImpl::DoTheStore(void* _obj)
{
    if (!_obj)  return -1;

    CBlockCaptureImpl* capture = (CBlockCaptureImpl*)_obj;

again:
    while (capture->m_captureState != CAPTURE_STARTING)
    {
        SleepSec(1);
        if (!g_env->enable())
            return -1;
    }

    if (!capture->preparePacketRing())
    {
        RM_LOG_ERROR("Fail to prepare packet ring.");
        capture->m_captureState = CAPTURE_STOPPED;
        return -1;
    }

    CaptureConfig_t config;
    if (0 != capture->prepareResource(config))
    {
        RM_LOG_ERROR("Fail to prepare the storage files.");
        capture->m_captureState = CAPTURE_STOPPED;
        return -1;
    }
    capture->m_savePktCount = 0;
    capture->m_saveByteCount = 0;
    capture->m_captureState = CAPTURE_STARTED;

    bool timeLimit = g_env->m_config.engine.captureDuration != 0;
    u_int64 tickCount = 0;
    time_t  prevCheckTime;
    time(&prevCheckTime);

    int z = 0;
    while (g_env->enable())
    {
        time_t now;
        time(&now);

        if (now - prevCheckTime >= 1)
        {
            prevCheckTime = now;
            capture->flushFile(++tickCount);

            if (timeLimit && tickCount >= g_env->m_config.engine.captureDuration)
            {
                RM_LOG_INFO("Time is up: " << g_env->m_config.engine.captureDuration);
                break;
            }
        }

        z = capture->startStoreImpl();
        if (z == 0)
            YieldCurThread();

        if (capture->m_captureState == CAPTURE_STOPPING)
        {
            // Flush all blocks left.
            capture->startStoreImpl();
            capture->finalize();
            capture->m_captureState = CAPTURE_STOPPED;
            goto again;
        }
    }

    capture->startStoreImpl();
    capture->finalize();
    capture->m_captureState = CAPTURE_STOPPED;

    RM_LOG_INFO("Store thread is ready to exit.");
    return 0;
}

int64 CBlockCaptureImpl::saveData(DataBlock_t* _dataBlock, u_int32 _flag)
{
    u_int64 offset = m_pcapFile->getOffset();
    assert(offset >= sizeof(pcap_file_header) && offset <= g_env->m_config.storage.fileSize);
    if (offset < sizeof(pcap_file_header) || offset > g_env->m_config.storage.fileSize)
    {
        RM_LOG_ERROR("File offset is wrong = " << offset);
        return -1;
    }

    if (m_pcapFile->getFileType() == FILE_PCAP)
        _dataBlock->dataDesc.blockSize = _dataBlock->dataDesc.usedSize;

    const byte* data = _dataBlock->dataDesc.pData;
    int dataLen = _dataBlock->dataDesc.blockSize;

    bool newFile = offset + dataLen > m_pcapFile->getFileSize();
    if (newFile)
    {
        int z = prepareNextFile();
        if (0 != z) return z;

        offset = m_pcapFile->getOffset();
    }
    assert(offset >= sizeof(pcap_file_header) && offset < g_env->m_config.storage.fileSize);

    _dataBlock->dataDesc.offset = offset;
    _dataBlock->dataDesc.resIndex = *m_curOutRes;
    _dataBlock->dataDesc.target = m_curTarget;
    _dataBlock->dataDesc.fileIndex = m_fileInfo->index;
    if (0 != m_pcapFile->write(data, dataLen, -1, this, _dataBlock, _flag))
    {
        RM_LOG_ERROR("Fail to save data " << m_pcapFile->getFileName() << ": " << m_pcapFile->getOffset() << ", " << dataLen);
        return -1;
    }

    m_fileInfo->packetCount += _dataBlock->dataDesc.pktCount;
    m_fileInfo->bytesCount += _dataBlock->dataDesc.usedSize - _dataBlock->dataDesc.pktCount * 16;

    assert(m_fileInfo->bytesCount < g_env->m_config.storage.fileSize);
    assert(m_fileInfo->packetCount < g_env->m_config.storage.fileSize / 64);

    return offset;
}

void CBlockCaptureImpl::switchTarget()
{
    if (g_env->m_config.storage.writeMode != WRITE_INTERLEAVED) return;

    ++m_curTarget %= g_env->m_config.storage.fileDir.size();
    m_fileInfo  = &g_env->m_config.storage.fileDir[m_curTarget].fileInfo;
    m_nextFileIndex = &g_env->m_config.storage.fileDir[m_curTarget].curFileIndex;
    m_curOutRes = &g_env->m_config.storage.fileDir[m_curTarget].outResIndex;
    m_pcapFile  = m_outRes[*m_curOutRes].pcapFile;
    m_curDB     = m_curTarget;
}

int CBlockCaptureImpl::saveBlock(DataBlock_t* _block)
{
    assert(m_pcapFile->isValid());

    if (g_env->m_config.engine.isSecAlign) // DIRECT IO need size and address sector alignment
    {
        assert(_block->dataDesc.blockSize % SECTOR_ALIGNMENT == 0);
        assert((u_int64)_block->dataDesc.pData % SECTOR_ALIGNMENT == 0);
    }
    u_int64 firstPacketTime = _block->dataDesc.firstPacketTime;
    u_int64 lastPacketTime = _block->dataDesc.lastPacketTime;
    int64 z = saveData(_block);     // on success will return the offset
    if (z < 0)
    {
        RM_LOG_ERROR("Fail to save block: " << _block->dataDesc.firstPacketTime);
        releaseBlockResource(_block);
        return z;
    }

    if (g_env->m_config.debugMode >= DEBUG_BLOCK)
    {
        struct timeval ts;
        gettimeofday(&ts, NULL);
        m_fsSaveBlock << ts.tv_sec << "." << ts.tv_usec << ": " \
            << _block->dataDesc.id << ", " \
            << (void*)_block << ", " \
            << _block->dataDesc.usedSize << ", "    \
            << _block->dataDesc.firstPacketTime << ", " \
            << _block->dataDesc.offset << endl;
    }

    m_fileInfo->lastPacketTime = MyMax(lastPacketTime, m_fileInfo->lastPacketTime);
    if (m_fileInfo->firstPacketTime == 0)
        m_fileInfo->firstPacketTime = firstPacketTime;

    assert(lastPacketTime >= firstPacketTime);
    assert(m_fileInfo->lastPacketTime >= m_fileInfo->firstPacketTime);
    //assert(firstPacketTime >= m_fileInfo->firstPacketTime &&
    //       lastPacketTime <= m_fileInfo->lastPacketTime);

    ++m_launchWriteCount;

    return 0;
}

int CBlockCaptureImpl::prepareNextFile()
{
    //
    // padding the file to make wire-shark happy
    //
    u_int64 fileSize = m_pcapFile->getFileSize();
    u_int64 offset = m_pcapFile->getOffset();

    int padSize = fileSize - offset;
    if (padSize > 0 && m_pcapFile->needPadding())
    {
        assert((u_int32)padSize <= DATA_BLOCK_SIZE);

        static DataBlock_t  padBlock;
        padBlock.dataDesc.usedSize = 0;
        padBlock.dataDesc.resIndex = *m_curOutRes;
        padding(&padBlock, padSize);
        assert(padBlock.dataDesc.usedSize == (u_int32)padSize);
        padBlock.dataDesc.pktCount = 0;
        if (0 != m_pcapFile->write(padBlock.data, padBlock.dataDesc.usedSize, -1, this, &padBlock)) // write the padding fake packet
            RM_LOG_ERROR("Fail to padding file: " << m_pcapFile->getFileName());
    }

    //
    // stop capture when about to wrap
    //
    int maxFileIndex = GetMaxFileIndex(m_curTarget);
    int nextFileIndex = (*m_nextFileIndex) % maxFileIndex;
    TargetConf_t& targetConf = g_env->m_config.storage.fileDir[m_curDB];

    if (g_env->m_config.capture.stopWhenWrap &&
       (nextFileIndex == targetConf.firstFileIndex &&
        targetConf.outResIndex >= 0))
    {
        RM_LOG_INFO("Stop on wrap.");
        return -2;  // wrap happen
    }

    if (0 != nextFile(true))
    {
        RM_LOG_ERROR("Fail to switch to the next file.");
        return -1;
    }

    return 0;
}

int CBlockCaptureImpl::saveBlock(PACKET_BLOCK_LIST& _blockList)
{
    u_int64 offset = m_pcapFile->getOffset();
    const byte* dataArray[512] = { 0 };
    int   dataLen[512] = { 0 };
    void* context[512] = { 0 };
    u_int64 dataOffset[512] = { 0 };
    int   blockCount = 0;

    PACKET_BLOCK_LIST::iterator it = _blockList.begin();
    for (; it != _blockList.end(); ++it)
    {
        DataBlock_t* block = *it;
        if (offset + block->dataDesc.usedSize > g_env->m_config.storage.fileSize)   // new file
        {
            if (blockCount > 0)
            {
                m_pcapFile->write(blockCount, dataArray, dataLen, dataOffset, this, context, 0);
            }

            int z = prepareNextFile();
            if (0 != z) return z;

            offset = m_pcapFile->getOffset();
            blockCount = 0;
        }

        const byte* data = block->dataDesc.pData;
        block->dataDesc.offset = offset;
        block->dataDesc.resIndex = *m_curOutRes;
        block->dataDesc.target = m_curTarget;

        dataArray[blockCount] = data;
        dataLen[blockCount] = block->dataDesc.usedSize;
        dataOffset[blockCount] = offset;
        context[blockCount] = block;

        ++blockCount;
        offset += block->dataDesc.usedSize;

        m_fileInfo->lastPacketTime = MyMax(block->dataDesc.lastPacketTime, m_fileInfo->lastPacketTime);
        if (m_fileInfo->firstPacketTime == 0)
            m_fileInfo->firstPacketTime = block->dataDesc.firstPacketTime;

        m_fileInfo->packetCount += block->dataDesc.pktCount;
        m_fileInfo->bytesCount += block->dataDesc.usedSize - block->dataDesc.pktCount * 16;
    }

    if (blockCount > 0)
    {
        m_pcapFile->write(blockCount, dataArray, dataLen, dataOffset, this, context, 0);
    }

    return 0;
}

void CBlockCaptureImpl::onTimer()
{
    static u_int32 secTick = 0;

    int step = secTick % 4;
    const char *charStep = "";
    if (step == 0)
        charStep = "-";
    else if (step == 1)
        charStep = "\\";
    else if (step == 2)
        charStep = "|";
    else if (step == 3)
        charStep = "/";

    cerr << charStep << "\r";
    if (secTick % 3 == 0)
    {
        time_t now;
        time(&now);
        updateStatistic(now, charStep);
    }

    if (g_env->enable())
    {
        m_Timer.expires_from_now(boost::posix_time::seconds(1));
        m_Timer.async_wait(boost::bind(&CBlockCaptureImpl::onTimer, this));
    }

    ++secTick;
}

int CBlockCaptureImpl::DoTheTimer(void* _obj)
{
    if (!_obj)  return -1;
    CBlockCaptureImpl* capture = (CBlockCaptureImpl*)_obj;
    while (!capture->isOpen())
    {
        SleepSec(1);
        if (!g_env->enable())
            return -1;
    }

    capture->m_TimerIO.run();
    return 0;
}

int CBlockCaptureImpl::DoTheProcess(void* _obj)
{
    if (!_obj)  return -1;

    CBlockCaptureImpl* capture = (CBlockCaptureImpl*)_obj;
    bool again = false;
    bool noParsePkt = !g_env->m_config.engine.enableParsePacket;
    do
    {
        again = false;
        noParsePkt = !g_env->m_config.engine.enableParsePacket;
        /**
         * if parse packet is disable, blocks will be saved directly by the
         * handlePacket thread
         */
        while (noParsePkt || capture->m_captureState != CAPTURE_STARTED)
        {
            SleepSec(1);
            if (!g_env->enable())
                return -1;

            // Get latest value of parse flag, which might be changed in
            // DoTheStore thread.
            noParsePkt = !g_env->m_config.engine.enableParsePacket;
        }

        CProducerRingPtr pktRing = capture->m_packetRing;
        assert(pktRing);
        RM_LOG_INFO("Total meta count = " << pktRing->GetMetaCount());
        RM_LOG_INFO("Total meta block count = " << pktRing->GetMetaBlockCount());

        int idleTime = 0;
        u_int32 blockPos = 0;
        while (g_env->enable())
        {
            PktMetaBlk_t *metaBlock = pktRing->GetNextFullMetaBlk();
            if (!metaBlock)
            {
                if (++idleTime % 1000 == 0);//  SleepMS(MyMin(idleTime / 1000, 1000));
                else YieldCurThread();

                continue;
            }
            //assert((blockPos++ % pktRing->GetMetaBlockCount()) == metaBlock->blkPos);
            idleTime = 0;
            bool stop = pktRing->IsStopSignal(metaBlock);
            bool timeout = pktRing->IsTimeoutSignal(metaBlock);
            if (!stop && !timeout)
                capture->processMetaBlock(metaBlock);
            pktRing->Move2NextMetaBlk();

            if (stop)
            {
                pktRing->FreeFullMetaBlk(metaBlock);

                again = true;
#ifdef DEBUG
                metaBlock = pktRing->GetNextFullMetaBlk();
                assert(!metaBlock);
#endif
                assert(pktRing->IsEmpty());
                pktRing->Stop();
                break;
            }
            else if (timeout)
            {
                pktRing->FreeFullMetaBlk(metaBlock);
                // XXX do something on timeout?
            }
        }
    } while (again);

    if (capture->m_appWarProcess)
    {
        KillProcess(capture->m_appWarProcess);
        capture->m_appWarProcess = (process_t)NULL;
    }

    RM_LOG_INFO("Process thread is ready to exit.");
    return 0;
}

void CBlockCaptureImpl::processMetaBlock(PktMetaBlk_t *_metaBlk)
{
#if defined(_DEBUG) || defined(DEBUG)
    for (u_int32 cursor = 0; cursor < _metaBlk->size; ++cursor)
    {
        PacketMeta_t* meta = m_packetRing->GetPktMetaInBlk(_metaBlk, cursor);
        packet_header_t header;
        packet_header_t* pktHeader = m_packetRing->GetPacketHeader(_metaBlk->type, meta, &header);
        if (pktHeader)
            assert(meta->basicAttr.ts == (u_int64)pktHeader->ts.tv_sec * NS_PER_SECOND + pktHeader->ts.tv_nsec);
        else   // fake packet
            assert(meta->basicAttr.pktLen == 1 && meta->basicAttr.ts == 0);

        if (g_env->m_config.debugMode >= DEBUG_PACKET && pktHeader)
        {
            m_fsSavePacket << meta->basicAttr.ts << ", "  \
                << (void*)meta->headerAddr << ", "  \
                << pktHeader->caplen << ", " << endl;
        }
    }
#endif

    if (_metaBlk->isFinal)
    {
        blockArrived(_metaBlk->dataBlk);    // let doStore_ to do the real job
    }
}

/**
* @Function: Insert the file header at the file's first block
**/
DataBlock_t* CBlockCaptureImpl::requestBlock(u_int32 _captureId)
{
    static atomic_t requestCount = -1;
    DataBlock_t* block = m_bufPool[_captureId].getBlock();
    if (block)
    {
        bzero(&block->dataDesc, sizeof(DataBlockDesc_t));
        block->dataDesc.pData = block->data;
        block->dataDesc.id = INTERLOCKED_INCREMENT(&requestCount);
        block->dataDesc.captureId = _captureId;
        block->dataDesc.type = PACKET_PCAP;
        block->dataDesc.capObject = this;
        block->dataDesc.blockSize = DATA_BLOCK_SIZE;
    }

    return block;
}

PacketMeta_t *CBlockCaptureImpl::requestMeta4PktBlk(u_int32 _captureId, DataBlockHeader_t *dataBlk)
{
    return m_packetRing->GetMeta4PktBlk(_captureId, (DataBlock_t*)dataBlk);
}

u_int32 CBlockCaptureImpl::getCurMetaBlkPos(u_int32 _captureId)
{
    return m_packetRing->GetCurMetaBlkPos(_captureId);
}

void CBlockCaptureImpl::finalMetaBlk(u_int32 _captureId)
{
    return m_packetRing->FinalMetaBlk(_captureId);
}

void CBlockCaptureImpl::sendStopSignal()
{
    m_packetRing->SendStopSignal();
}

int CBlockCaptureImpl::releaseBlock(DataBlock_t* _block)
{
    if (_block) m_bufPool[_block->dataDesc.captureId].release(_block);
    return 0;
}

int CBlockCaptureImpl::blockArrived(DataBlock_t* _block, int _blockCount)
{
    if (!_block)    return -1;

    //
    // if there are multiple capture object means there will be multiple thread call this
    // so use a separate thread to do the saving operation,
    // otherwise just save the block at once
    //
    SCOPE_LOCK(m_blockListMutex);
    m_blockList.push_back(_block);
    return 0;
}

void CBlockCaptureImpl::regObj(CNetCapture* _obj)
{
    if (_obj && _obj != this)
    {
        SCOPE_LOCK(m_capMapMutex);
        m_capObjMap[_obj->getDevIndex()] = _obj;
    }
}

void CBlockCaptureImpl::unregObj(CNetCapture* _obj)
{
    if (_obj)
    {
        SCOPE_LOCK(m_capMapMutex);
        m_capObjMap.erase(_obj->getDevIndex());
    }
}

CNetCapture* CBlockCaptureImpl::getCapObj(int _index)
{
    if (_index == getDevIndex())    return this;

    SCOPE_LOCK(m_capMapMutex);
    if (m_capObjMap.count(_index))
        return m_capObjMap[_index];

    return NULL;
}

/************************************************************************/
// The following function are used to query and update file information
// directly on the file database
/************************************************************************/
int CBlockCaptureImpl::GetRecord_Callback(void* _context, int _argc, char** _argv, char** _szColName)
{
    FileInfo_t fileInfo = {0};
    vector<FileInfo_t>* vec = (vector<FileInfo_t>*) _context;

    for (int i = 0; i < _argc; ++i)
    {
        if (0 == stricmp(_szColName[i], "NAME"))
            fileInfo.index = atoi(_argv[i]);
        else if (0 == stricmp(_szColName[i], "AGE"))
            fileInfo.age = atoi(_argv[i]);
        else if (0 == stricmp(_szColName[i], "FIRST_PACKET_TIME"))
            fileInfo.firstPacketTime = atoi64(_argv[i]);
        else if (0 == stricmp(_szColName[i], "LAST_PACKET_TIME"))
            fileInfo.lastPacketTime = atoi64(_argv[i]);
        else if (0 == stricmp(_szColName[i], "STATUS"))
            fileInfo.status = atoi(_argv[i]);
        else if (0 == stricmp(_szColName[i], "SIZE"))
            fileInfo.size = atoi64(_argv[i]);
        else if (0 == stricmp(_szColName[i], "COUNT(*)") ||
                 0 == stricmp(_szColName[i], "COUNT(1)") ||
                 0 == stricmp(_szColName[i], "SUM(PACKET_COUNT)") ||
                 0 == stricmp(_szColName[i], "MIN(FIRST_PACKET_TIME)") ||
                 0 == stricmp(_szColName[i], "MAX(LAST_PACKET_TIME)"))
        {
            if (_context && _argv[i])
            {
                u_int64* value = (u_int64*)_context;
                *value = atoi64(_argv[i]);
            }
            return 0;
        }
    }

    if (vec)    vec->push_back(fileInfo);
    return 0;
}

u_int64 CBlockCaptureImpl::getCapacity()
{
    u_int64 capacity = 0;
    for (unsigned int i = 0; i < g_env->m_config.storage.fileDir.size(); ++i)
        capacity += g_env->m_config.storage.fileDir[i].capacity / ONE_GB;

    return capacity;
}

u_int64 CBlockCaptureImpl::getTotalPacketCount()
{
    vector<u_int64> vec;
    u_int64 pktCount = 0;
    queryRecordDB("SELECT SUM(PACKET_COUNT) FROM " FILE_STATUS_TABLE, vec);

    for (unsigned int i = 0; i < vec.size(); ++i)
        pktCount += vec[i];
    return pktCount;
}

u_int64 CBlockCaptureImpl::getFirstPacketTime()
{
    vector<u_int64> vec;
    u_int64 firstPacketTime = -1;
    queryRecordDB("SELECT MIN(FIRST_PACKET_TIME) FROM " FILE_STATUS_TABLE \
                  " WHERE FIRST_PACKET_TIME > 0", vec);
    for (unsigned int i = 0; i < vec.size(); ++i)
        firstPacketTime = MyMin(firstPacketTime, vec[i]);

    return firstPacketTime;
}

u_int64 CBlockCaptureImpl::getFirstNonLockedPktTime()
{
    vector<u_int64> vec;
    u_int64 firstPacketTime = -1;
    CStdString query;
    query.Format("SELECT MIN(FIRST_PACKET_TIME) FROM " FILE_STATUS_TABLE \
                 " WHERE STATUS != %d AND FIRST_PACKET_TIME > 0", STATUS_LOCKED);
    queryRecordDB(query, vec);
    for (unsigned int i = 0; i < vec.size(); ++i)
        firstPacketTime = MyMin(firstPacketTime, vec[i]);

    return firstPacketTime;
}

u_int64 CBlockCaptureImpl::getLastPacketTime()
{
    vector<u_int64> vec;
    u_int64 lasttPacketTime = 0;
    queryRecordDB("SELECT MAX(LAST_PACKET_TIME) FROM " FILE_STATUS_TABLE, vec);
    for (unsigned int i = 0; i < vec.size(); ++i)
        lasttPacketTime = MyMax(lasttPacketTime, vec[i]);

    return lasttPacketTime;
}

double CBlockCaptureImpl::getDiskUsage()
{
    u_int64 totalFileCount = g_env->m_config.storage.fileCount;
    u_int64 fileCount = 0;
    vector<u_int64> vec;

    queryRecordDB("SELECT COUNT(*) FROM " FILE_STATUS_TABLE " WHERE SIZE > 0", vec);
    for (unsigned int i = 0; i < vec.size(); ++i)
        fileCount += vec[i];

    return (double)fileCount / totalFileCount;
}

double CBlockCaptureImpl::getLockedDisk()
{
    return (double)getLockedFileCount() / g_env->m_config.storage.fileCount;
}

u_int64 CBlockCaptureImpl::getLockedFileCount()
{
    u_int64 lockFileCount = 0;
    vector<u_int64> vec;

    CStdString query;
    query.Format("SELECT COUNT(1) FROM " FILE_STATUS_TABLE " WHERE STATUS = %d", STATUS_LOCKED);
    queryRecordDB(query, vec);
    for (unsigned int i = 0; i < vec.size(); ++i)
        lockFileCount += vec[i];

    return lockFileCount;
}

int CBlockCaptureImpl::lockFile(u_int64 _firstTime, u_int64 _lastTime)
{
    CStdString query;
    query.Format("SELECT COUNT(1) FROM " FILE_STATUS_TABLE \
                  " WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
                  _lastTime, _firstTime);
    vector<u_int64> vec;
    int z = queryRecordDB(query, vec);
    if (z != 0) return -2;
    u_int64 fileCount = std::accumulate(vec.begin(), vec.end(), 0);
    if (fileCount == 0) return 0;

    u_int64 lockedFileCount = fileCount + getLockedFileCount();
    const double maxLockRatio = 0.8; // 80% percent
    u_int64 maxLockedFileCount = g_env->m_config.storage.fileCount * maxLockRatio;
    if (lockedFileCount > maxLockedFileCount)
        return -1; // Cannot lock anymore, otherwise S2D has no place to write packets.

    query.Format("UPDATE " FILE_STATUS_TABLE " SET STATUS=%d" \
                 " WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
                 STATUS_LOCKED, _lastTime, _firstTime);
    z = execOnRecordDB(query, NULL, NULL);
    if (z != 0) return -2;

    TimeSegment timeSeg;
    timeSeg.startTime = _firstTime;
    timeSeg.stopTime = _lastTime;
    g_env->m_config.capture.lockTime.push_back(timeSeg);

    return fileCount;
}

int CBlockCaptureImpl::unlockFile(u_int64 _firstTime, u_int64 _lastTime)
{
    CStdString query;
    query.Format("SELECT COUNT(1) FROM " FILE_STATUS_TABLE \
                  " WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
                  _lastTime, _firstTime);
    vector<u_int64> vec;
    int z = queryRecordDB(query, vec);
    if (z != 0) return -2;
    u_int64 fileCount = std::accumulate(vec.begin(), vec.end(), 0);
    if (fileCount == 0)
    {
        RM_LOG_ERROR("Time range " << _firstTime << " - " << _lastTime
                     << " does NOT have any locked packets");
        return 0;
    }

    query.Format("UPDATE " FILE_STATUS_TABLE " SET STATUS=%d WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
        STATUS_NORMAL, _lastTime, _firstTime);
    z = execOnRecordDB(query, NULL, NULL);
    if (z != 0) return -2;

    TimeSegment timeSeg;
    timeSeg.startTime = _firstTime;
    timeSeg.stopTime = _lastTime;

    for (unsigned int i = 0; i < g_env->m_config.capture.lockTime.size(); ++i)
    {
        if (timeSeg == g_env->m_config.capture.lockTime[i])
        {
            g_env->m_config.capture.lockTime.erase(g_env->m_config.capture.lockTime.begin() + i);
            break;
        }
    }

    return fileCount;
}

int CBlockCaptureImpl::ClearFile(FileInfo_t* _fileInfo, void* _context)
{
    if (!_fileInfo) return -1;

    int fileIndex = _fileInfo->index;
    int target = 0;
    int count = g_env->m_config.storage.fileDir[target].fileCount;

    while (count < fileIndex)
    {
        count += g_env->m_config.storage.fileDir[++target].fileCount;
    }

    AdjustFileIndex(target, fileIndex);

    CStdString fileName;
    if (g_env->m_config.storage.renameFile)   // don't update
    {
        fileName.Format("%04d-" I64D "-" I64D "-%d-%d-" I64D ".pcap",
            fileIndex % g_filesPerDir,    // there may no record, so use m_nextFileIndex rather than m_fileInfo->index
            _fileInfo->firstPacketTime,
            _fileInfo->lastPacketTime,
            _fileInfo->status,
            _fileInfo->age,
            _fileInfo->size);
    }
    else
    {
        fileName.Format("%04d-0-0-0-0-0.pcap", fileIndex % g_filesPerDir);
    }

    // Clear block info
    CStdString targetName;
    targetName.Format("target%d/", GetTargetDBIndex(target));
    CStdString blkDBName;
    blkDBName.Format("%s/%s%s%04d_file_blk.dbr",
                     g_env->m_config.engine.dbPath,
                     targetName.c_str(),
                     GetFileSubDirByIndex(fileIndex, g_env->m_config.storage.dirLevel),
                     fileIndex % g_filesPerDir);
    CBlockFile blkDB;
    blkDB.setFileName(blkDBName);
    blkDB.open(ACCESS_WRITE, FILE_TRUNCATE_EXISTING, false, false);
    blkDB.close();

    //CStdString filePath = GetFilePathByIndex(_fileInfo->index, 2);
    //filePath += "/";
    //filePath += fileName;

    //CPCAPFile   file;
    //if (0 == file.open(filePath, false))
    //{
    //    u_int64 fileSize = file.getFileSize();
    //    byte* fileData = file.getBuf();
    //    bzero(fileData, fileSize);
    //    file.close();
    //}

    return 0;
}

int CBlockCaptureImpl::deletePacket(u_int64 _firstTime, u_int64 _lastTime)
{
    TimeSegment timeSeg;
    timeSeg.startTime = _firstTime;
    timeSeg.stopTime = _lastTime;

    vector<FileInfo_t> fileInfoList;
    int z = getFileInfoList(_firstTime, _lastTime, fileInfoList);
    if (z != 0) return -2;
    if (fileInfoList.size() == 0)
    {
        RM_LOG_INFO("Trying to delete packet between " << _firstTime
                    << " - " << _lastTime << ", which contains no packet");
        return 0;
    }
    BOOST_FOREACH (const FileInfo_t &info, fileInfoList)
    {
        if (info.status != STATUS_NORMAL)
        {
            RM_LOG_WARNING("Trying to delete packet between "
                           << _firstTime << " - " << _lastTime
                           << ", whose status is not 0 but " << info.status);
            return -1;
        }
    }

    CStdString query;
    query.Format("UPDATE " FILE_STATUS_TABLE " SET STATUS=%d"\
                 " WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
                 STATUS_DELETING, _lastTime, _firstTime);
    z = execOnRecordDB(query, NULL, NULL);
    if (z != 0) return -2;

    query.Format("SELECT * FROM " FILE_STATUS_TABLE " WHERE STATUS=%d", STATUS_DELETING);
    z = execOnRecordDB(query, ClearFile, NULL);
    if (z != 0) return -2;

    query.Format("UPDATE " FILE_STATUS_TABLE \
                 " SET STATUS=%d, FIRST_PACKET_TIME=0, LAST_PACKET_TIME=0, SIZE=0 WHERE STATUS=%d",
                 STATUS_NORMAL, STATUS_DELETING);
    z = execOnRecordDB(query, NULL, NULL);
    if (z != 0) return -2;

    return z;
}

CStdString CBlockCaptureImpl::getCurrentFile()
{
    return m_pcapFile ? m_pcapFile->getFileName() : "";
}

int CBlockCaptureImpl::queryRecordDB(const tchar* _sql, vector<u_int64>& _result)
{
    int z = 0;
    CStdString dbPath;

    for (int i = 0; i < GetRecordDBCount(); ++i)
    {
        CStdString targetName;
        targetName.Format("target%d", i);
        dbPath.Format("%s/%s/farewell.db", g_env->m_config.engine.dbPath, targetName.c_str());

        CSqlLiteDB db;
        if (0 == db.open(dbPath))
        {
            int z = -1;
            int retry = 0;

            while (z != 0 && retry++ < 3)
            {
                u_int64 count = 0;
                z = db.exec(_sql, &count, GetRecord_Callback);
                if (0 != z)
                {
                    RM_LOG_ERROR("Fail to execute: " << _sql);
                    SleepSec(1);
                }
                else
                    _result.push_back(count);
            }
        }
    }

    return z;
}

int CBlockCaptureImpl::execOnRecordDB(const tchar* _sql, RecordFileFunc _func, void* _context)
{
    int z = 0;
    CStdString dbPath;

    for (int i = 0; i < GetRecordDBCount(); ++i)
    {
        CStdString targetName;
        targetName.Format("target%d", i);
        dbPath.Format("%s/%s/farewell.db", g_env->m_config.engine.dbPath, targetName.c_str());

        CSqlLiteDB db;
        if (0 == db.open(dbPath))
        {
            int z = -1;
            int retry = 0;
            vector<FileInfo_t> vec;

            while (z != 0 && retry++ < 3)   // retry 3 times when on error
            {
                z = db.exec(_sql, &vec, GetRecord_Callback);
                if (0 != z)
                {
                    RM_LOG_ERROR("Fail to execute: " << _sql);
                    SleepSec(1);
                }
                else if (_func)
                {
                    for (unsigned int i = 0; i < vec.size(); ++i)
                    {
                        _func(&vec[i], _context);
                    }
                }
            }
        }
    }

    return z;
}

int CBlockCaptureImpl::getFileInfoList(u_int64 _firstTime, u_int64 _lastTime,
                                       vector<FileInfo_t> &files)
{
    CStdString query;
    query.Format("SELECT * FROM " FILE_STATUS_TABLE \
                  " WHERE FIRST_PACKET_TIME <= " I64D " AND LAST_PACKET_TIME >= " I64D,
                  _lastTime, _firstTime);
    for (int i = 0; i < GetRecordDBCount(); ++i)
    {
        CStdString targetName;
        targetName.Format("target%d", i);

        CStdString dbPath;
        dbPath.Format("%s/%s/farewell.db",
                      g_env->m_config.engine.dbPath, targetName.c_str());

        CSqlLiteDB db;
        if (0 == db.open(dbPath))
        {
            int z = -1;
            int retry = 0;

            while (z != 0 && retry++ < 3)   // retry 3 times when on error
            {
                z = db.exec(query, &files, GetRecord_Callback);
                if (0 != z)
                {
                    RM_LOG_ERROR("Fail to execute: " << query);
                    SleepSec(1);
                }
            }

            return z;
        }
    }

    return 0;
}

/************************************************************************/
//
/************************************************************************/

CProducerRingPtr CBlockCaptureImpl::preparePacketRing()
{
    assert(m_launchWriteCount == m_finishWriteCount);
    //
    // use default share object and register module "NetKeeper"
    // every port will have a shared memory block
    //
    if (m_packetRing)
    {
#ifdef DEBUG
        if (g_env->m_config.engine.enableParsePacket)
            assert(m_packetRing->IsStopped());
#endif
        m_packetRing.reset();
    }

    m_packetRing = CProducerRingPtr(new CProducerRing());
    m_packetRing->Clear(); // Clear first, in case something left in share memory
    m_packetRing->Init("", "NetKeeper", MAX_CAPTURE_THREAD * g_env->m_config.engine.blockMemSize);

    ModuleInfo_t* modInfo = (ModuleInfo_t*)m_packetRing->GetModuleInfo();
    if (!modInfo)
        return CProducerRingPtr();

    assert(m_packetRing->GetModuleID() == 0);   // make sure this is the first register module

    //
    // keep the registered model information and let the pool to manage the share memory
    //
    m_modInfo.blockViewStart = modInfo->blockViewStart;
    m_modInfo.metaViewStart = modInfo->metaViewStart;
    strcpy(m_modInfo.moduleName, modInfo->moduleName);

    u_int32 blockCount = g_env->m_config.engine.blockMemSize / sizeof(DataBlock_t);
    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        DataBlock_t* startAddr = (DataBlock_t*)(modInfo->blockViewStart + i * g_env->m_config.engine.blockMemSize);
        m_bufPool[i].resetMem(startAddr, blockCount);
    }

    RM_LOG_INFO_S("Success to register the module '" << modInfo->moduleName << "' (" << m_packetRing->GetModuleID() << ")");
    RM_LOG_INFO_S("Block count = " << blockCount);

    m_appWarProcess = (process_t)NULL;
    if (g_env->m_config.engine.enableParsePacket)
    {
        loadAppWars();
    }
    m_packetRing->Start();

    return m_packetRing;
}

int CBlockCaptureImpl::startCapture()
{
    for (unsigned int i = 0; i < m_localNICs.size(); ++i)
    {
        startCapture(i);
    }

    return 0;
}

int CBlockCaptureImpl::startCapture(int _index, CaptureConfig_t& _config)
{
    if ((unsigned int)_index >= m_localNICs.size())
    {
        RM_LOG_ERROR("The NIC index " << _index << " is out of value.");
        return -1;
    }

    CNetCapture* devCapture = getCapObj(_index);
    DeviceST& device = m_localNICs[_index];

    if (!devCapture)
    {
        switch (device.type)
        {
        case DEVICE_TYPE_LIBPCAP:
            devCapture = new CPcapCapture;
            break;
        case DEVICE_TYPE_VIRTUAL:
            devCapture = new CVirtualCapture;
            break;
        default:
            break;
        }
        assert(devCapture);
        if (!devCapture)   return -1;

        if (0 != devCapture->openDevice(_index))
        {
            RM_LOG_ERROR("Fail to open device: " << device.desc);
            delete devCapture;
            devCapture = NULL;
            return -1;
        }
    }

    int z = devCapture->startCapture(_config);
    if (0 == z && m_captureState != CAPTURE_STARTED)
    {
        m_captureState = CAPTURE_STARTING;
    }

    return z;
}

int CBlockCaptureImpl::startCapture(int _index, const tchar* _filter)
{
    if ((unsigned int)_index >= m_localNICs.size())
    {
        RM_LOG_ERROR("The NIC index " << _index << " is out of value.");
        return -1;
    }

    CNetCapture* devCapture = getCapObj(_index);
    DeviceST& device = m_localNICs[_index];

    if (!devCapture)
    {
        switch (device.type)
        {
        case DEVICE_TYPE_LIBPCAP:
            devCapture = new CPcapCapture;
            break;
        case DEVICE_TYPE_VIRTUAL:
            devCapture = new CVirtualCapture;
            break;
        default:
            break;
        }
        assert(devCapture);
        if (!devCapture)   return -1;

        if (0 != devCapture->openDevice(_index))
        {
            RM_LOG_ERROR("Fail to open device: " << device.desc);
            delete devCapture;
            devCapture = NULL;
            return -1;
        }
    }

    if (_filter && *_filter)
        devCapture->setCaptureFilter(_filter, 0xF);


    CaptureConfig_t config;
    int z = devCapture->startCapture(config);
    if (0 == z && m_captureState != CAPTURE_STARTED)
        m_captureState = CAPTURE_STARTING;

    return z;
}

int CBlockCaptureImpl::stopCapture()
{
    if (m_captureState == CAPTURE_STOPPED)  return 0;

    //
    // stop all the device capturing first
    //
    while (m_capObjMap.size() > 0)
    {
        map<int, CNetCapture*>::const_iterator it = m_capObjMap.begin();
        for (; it != m_capObjMap.end(); ++it)
        {
            CNetCapture* capture = it->second;
            int devIndex = capture->getDevIndex();
            capture->stopCapture();
            capture->closeDevice();
            SAFE_DELETE(capture);

            break;  // since capture->closeDevice() will erase the object from m_capObjMap, so redo
        }
    }

    m_captureState = CAPTURE_STOPPING;  // notify DoTheStore() to stop saving

    RM_LOG_INFO("Stop capturing: wait all the io finished.");
    while (m_finishWriteCount != m_launchWriteCount)    // wait until all the io are finished
        SleepSec(1);

    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
        m_bufPool[i].stop(true);        // avoid device capturer deadlock in the requestBlock
    RM_LOG_INFO("Stop capturing: all the io are finished.");

    if (g_env->m_config.engine.enableParsePacket)
    {
        sendStopSignal();   // all the NIC stopped, now notify the ring to stop
        while (!m_packetRing->IsStopped())
            SleepSec(1);
    }

    g_env->m_config.capture.fileStatus = STATUS_NORMAL;
    return 0;
}

int CBlockCaptureImpl::stopCapture(int _index)
{
    if ((unsigned int)_index >= m_localNICs.size())
    {
        RM_LOG_ERROR("No availabe device index: " << _index);
        return -1;
    }

    CNetCapture* capObj = getCapObj(_index);
    if (!capObj)
    {
        RM_LOG_ERROR("The device is not capturing.");
        return -1;
    }

    int z = capObj->stopCapture();      // will block to wait for the device to stop capturing
    capObj->closeDevice();
    delete capObj;

    DeviceST& device = m_localNICs[_index];
    RM_LOG_INFO("Success to stop capture [" << _index << "]: " << device.name);

    {
        //
        // if all the NIC are stopped, stop the store
        //
        SCOPE_LOCK(m_capMapMutex);
        map<int, CNetCapture*>::const_iterator it = m_capObjMap.begin();
        for (; it != m_capObjMap.end(); ++it)
        {
            CNetCapture* capture = it->second;
            if (capture->getCaptureState() != CAPTURE_STOPPED)
                break;
        }

        if (it == m_capObjMap.end())
        {
            RM_LOG_INFO("Prepare to stop storing.");
            stopCapture();
            while (m_captureState != CAPTURE_STOPPED)
                YieldCurThread();
            RM_LOG_INFO("Success to stop storing.");
        }
    }

    return z;
}

int CBlockCaptureImpl::setFilter(int _index, int _port, const tchar* _filter)
{
    CNetCapture* capObj = getCapObj(_index);
    if (!capObj)  return -1;

    return capObj->setCaptureFilter(_filter, _port);
}

DeviceST* CBlockCaptureImpl::getDevice(int _index)
{
    if ((unsigned int)_index >= m_localNICs.size())   return NULL;
    return &m_localNICs[_index];
}

ICaptureFile* CBlockCaptureImpl::createCaptureFile()
{
    CPCAPFile2* pFile = NULL;

    if (g_env->m_config.engine.enableS2Disk)
    {
        switch (g_env->m_config.storage.fileType)
        {
        default:
            pFile = new CPCAPFile2(&m_filePool, g_env->m_config.engine.isSecAlign);
            break;
        }
    }
    else
    {
        pFile = new CFakePcapFile(&m_filePool, g_env->m_config.engine.isSecAlign);
    }

    return pFile;
}

int CBlockCaptureImpl::nextFile(bool _next)
{
    u_int32 retry = 0;

    //
    // update the information
    //
    if (m_pcapFile && m_pcapFile->isValid() && m_fileInfo->firstPacketTime > 0)
    {
        ++m_fileInfo->age;   // add one age
        m_fileInfo->status = g_env->m_config.capture.fileStatus;    // mostly, it should be STATUS_NORMAL or STATUS_LOCKED
        updateFileName();
    }

    if (_next)
    {
        if (g_env->m_config.storage.writeMode == WRITE_SEQUENTIAL)
            ++(*m_curOutRes) %= m_resPerTarget;
        else
        {
            int nextRes = ((*m_curOutRes + 1) % m_resPerTarget);
            *m_curOutRes = m_curTarget * m_resPerTarget + nextRes;
        }
    }

    assert(*m_curOutRes >= 0 && *m_curOutRes < m_outResCount);

    //
    // clear the remained information
    //
    OutRes_t& outRes = m_outRes[*m_curOutRes];
    outRes.pcapFile->close();
    outRes.blkInfoDB.close();
    outRes.pktMetaDB.close();

    m_pcapFile = outRes.pcapFile;

    //
    // skip at most 10 files if any exception happen
    //
    bool b = false;
    int z = 0;
    while (!b && retry <= MyMin((u_int32)10, g_env->m_config.storage.fileCount))
    {
        z = CNetCapture::nextFile(_next);
        if (z == 0)
        {
            if (0 != createFileDB(_next))
            {
                b = false;
                ++(*m_nextFileIndex);  // try the next file
                ++retry;
                continue;
            }

            b = true;
        }
        else if (z == -2)   // don't retry since all the files are locked or stop on wrap
        {
            break;
        }
        else
        {
            ++(*m_nextFileIndex);  // try the next file
            ++retry;
        }
    }

    return z;
}

int CBlockCaptureImpl::flushDB()
{
    //
    // flush the db, this should be done in the I/O recycle thread
    //
    for (int i = 0; i < m_outResCount; ++i)
    {
        OutRes_t& outRes = m_outRes[i];
        if (outRes.blkInfoDB.isValid()) outRes.blkInfoDB.flush();
        if (outRes.pktMetaDB.isValid()) outRes.pktMetaDB.flush();
    }

    return 0;
}

/**
 * 1. data files are belonged to store thread
 * 2. file db is belonged to store thread
 * 3. block db is belonged to aio thread
 *
 * @Warning: so please don't flushDB here
 *
 **/
int CBlockCaptureImpl::flushFile(u_int64 _tickCount)
{
    if (_tickCount % 10 == 0)  // every 10 seconds to update the file info
    {
        for (int i = 0; i < GetRecordDBCount(); ++i)
        {
            if (_tickCount == 0 && m_fileInfo->status == STATUS_CAPTURE)    // stop happened
            {
                ++m_fileInfo->age;
                m_fileInfo->status = g_env->m_config.capture.fileStatus;
            }

            m_pcapFile->flush();
            updateFileName();
            switchTarget();
        }
    }

    return 0;
}

int CBlockCaptureImpl::getStatistic(Statistic_t& _pcapStat)
{
    int z = 0;
    bzero(&_pcapStat, sizeof(_pcapStat));

    {
        SCOPE_LOCK(m_capMapMutex);
        map<int, CNetCapture*>::const_iterator it = m_capObjMap.begin();
        for (; it != m_capObjMap.end(); ++it)
        {
            CNetCapture* capture = it->second;
            Statistic_t stats = { 0 };
            if (0 == capture->getStatistic(stats))
            {
                _pcapStat.pktRecvCount += stats.pktRecvCount;
                _pcapStat.pktSeenByNIC += stats.pktSeenByNIC;
                _pcapStat.bytesRecvCount += stats.bytesRecvCount;
                _pcapStat.bytesSeenByNIC += stats.bytesSeenByNIC;
                _pcapStat.pktDropCount += stats.pktDropCount;
                _pcapStat.pktErrCount += stats.pktErrCount;
                _pcapStat.utilization += stats.utilization;
                ++z;
            }
        }
    }

    _pcapStat.utilization /= m_capObjMap.size();
    _pcapStat.pktSaveCount = m_savePktCount;
    _pcapStat.bytesSaveCount = m_saveByteCount;

    static u_int64 count = 0;
    if (count++ < 3 && _pcapStat.pktDropCount > 0)  // ignore the first dropped count
        _pcapStat.pktDropCount = 0;

    return z > 0 ? 0 : -1;
}

int CBlockCaptureImpl::getStatistic(Statistic_t& _pcapStat, int _port)
{
    int z = 0;
    bzero(&_pcapStat, sizeof(_pcapStat));

    {
        SCOPE_LOCK(m_capMapMutex);
        map<int, CNetCapture*>::const_iterator it = m_capObjMap.begin();
        for (; it != m_capObjMap.end(); ++it)
        {
            CNetCapture* capture = it->second;
            Statistic_t stats = { 0 };
            capture->lockDevice();  // prevent closing device
            if (0 == capture->getStatistic(stats, _port))
            {
                _pcapStat.pktRecvCount += stats.pktRecvCount;
                _pcapStat.pktSeenByNIC += stats.pktSeenByNIC;
                _pcapStat.bytesRecvCount += stats.bytesRecvCount;
                _pcapStat.bytesSeenByNIC += stats.bytesSeenByNIC;
                _pcapStat.pktDropCount += stats.pktDropCount;
                _pcapStat.pktErrCount += stats.pktErrCount;
                _pcapStat.utilization += stats.utilization;
                ++z;
            }
            capture->unlockDevice();
        }
    }

    return z;
}

int CBlockCaptureImpl::getStatistic(Statistic_t& _pcapStat, int _index, int _port)
{
    CNetCapture* capObj = getCapObj(_index);
    if (!capObj)  return -1;

    return capObj->getStatistic(_pcapStat, _port);
}

void CBlockCaptureImpl::constructExtIndices()
{
    m_extIndexSize.clear();

    if (g_env->m_config.engine.enableParsePacket)
    {
        vector<ModuleIndex_t> vec;
        m_packetRing->GetModuleIndex(vec);
        vector<ModuleIndex_t>::const_iterator it = vec.begin();
        for (; it != vec.end(); ++it)
        {
            for (int i = 0; i < it->colCount; ++i)
            {
                m_extIndexSize.push_back(it->colSize[i]);
            }
        }
    }
}

int CBlockCaptureImpl::createFileDB(bool _override)
{
    if (m_extIndexSize.empty())
    {
        constructExtIndices();
    }

    OutRes_t& outRes = m_outRes[*m_curOutRes];
    int z = 0;
    CStdString dbName;
    CStdString targetName;
    targetName.Format("target%d/", GetTargetDBIndex(m_curTarget));
    dbName.Format("%s/%s%s%04d_file.db",
                    g_env->m_config.engine.dbPath,
                    targetName.c_str(),
                    GetFileSubDirByIndex(m_fileInfo->index, g_env->m_config.storage.dirLevel),
                    m_fileInfo->index % g_filesPerDir);

    if (0 != outRes.blkInfoDB.open(dbName) ||
        0 != outRes.blkInfoDB.exec("PRAGMA synchronous = OFF", this, NULL) ||
        0 != outRes.blkInfoDB.exec("PRAGMA journal_mode = MEMORY", this, NULL))
    {
        RM_LOG_ERROR("Fail to open the db: " << dbName);
        return -1;
    }

    //
    // The table to keep block index information
    //
    int tableExist = outRes.blkInfoDB.isTableExist(FILE_BLOCK_TABLE);
    ON_ERROR_LOG_MESSAGE_AND_DO(tableExist, == , -1, FILE_BLOCK_TABLE "not exist", return -1);

    if (_override && tableExist)
    {
        CStdStringA sql = "DELETE FROM " FILE_BLOCK_TABLE;
        z = outRes.blkInfoDB.exec(sql, this, NULL);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, "Fail to delete existing records: " << sql, return z);
    }

    if (!tableExist)
    {
        CStdStringA sql = "CREATE TABLE "   \
            FILE_BLOCK_TABLE "("  \
            "ID INTEGER PRIMARY KEY, " \
            "FILE_INDEX         INTEGER, "  \
            "PORT_NUM           INTEGER,"  \
            "FRIST_TIMESTAMP    NUMERIC NOT NULL, " \
            "LAST_TIMESTAMP     NUMERIC NOT NULL, " \
            "PACKET_COUNT       INTEGER NOT NULL, " \
            "OFFSET             NUMERIC NOT NULL, " \
            "SIZE               INTEGER NOT NULL, " \
            "BLOCK_SIZE         INTEGER NOT NULL );";

        z = outRes.blkInfoDB.exec(sql, this, NULL);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, "Fail to create table: " << sql, return z);
    }

    if (!outRes.blkInfoDB.getStatment(FILE_BLOCK_TABLE))
    {
        CStdString sql = "INSERT INTO " FILE_BLOCK_TABLE " VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?);";
        z = outRes.blkInfoDB.compile(FILE_BLOCK_TABLE, sql);
    }

    z = outRes.blkInfoDB.beginTransact();
    return z;
}


int CBlockCaptureImpl::addPacketRecord(PacketMeta_t* _metaInfo, int _curOutRes)
{
    if (!_metaInfo) return -1;
    if (!g_env->m_config.engine.enableInsertDB) return 0;

    assert(_curOutRes < m_outResCount);
    OutRes_t& outRes = m_outRes[_curOutRes];

    int z = 0;
    if (_metaInfo->basicAttr.ts && _metaInfo->basicAttr.pktLen)
    {
        int totalExtSize = 0;
        if (m_extIndexSize.size() > 0)
            totalExtSize = std::accumulate(m_extIndexSize.begin(), m_extIndexSize.end(), 0);
        outRes.pktMetaDB.saveData((byte*)&_metaInfo->basicAttr, totalExtSize + sizeof(PacketBasicAttr_t));
    }

    return z;
}

int CBlockCaptureImpl::addBlockRecord(DataBlock_t* _block)
{
    OutRes_t& outRes = m_outRes[_block->dataDesc.resIndex];

    sqlite3_stmt* stmt = outRes.blkInfoDB.getStatment(FILE_BLOCK_TABLE);
    if (!_block || !stmt)    return -1;

    int z = sqlite3_reset(stmt);
    if (0 != z)
    {
        RM_LOG_ERROR(sqlite3_errmsg(outRes.blkInfoDB));
    }
    else if (0 == sqlite3_bind_int(stmt, 1, _block->dataDesc.fileIndex) &&
        0 == sqlite3_bind_int(stmt, 2, _block->dataDesc.portNum) &&
        0 == sqlite3_bind_int64(stmt, 3, _block->dataDesc.firstPacketTime) &&
        0 == sqlite3_bind_int64(stmt, 4, _block->dataDesc.lastPacketTime) &&
        0 == sqlite3_bind_int(stmt, 5, _block->dataDesc.pktCount) &&
        0 == sqlite3_bind_int64(stmt, 6, _block->dataDesc.offset) &&
        0 == sqlite3_bind_int(stmt, 7, _block->dataDesc.usedSize) &&
        0 == sqlite3_bind_int(stmt, 8, _block->dataDesc.blockSize))
    {
        z = sqlite3_step(stmt);
        if (0 != z && SQLITE_DONE != z)
        {
            RM_LOG_ERROR(sqlite3_errmsg(outRes.blkInfoDB));
        }
    }

    return z;
}

int CBlockCaptureImpl::startStoreImpl()
{
    if (m_blockList.size() == 0) return 0;

    PACKET_BLOCK_LIST blockList;
    {
        SCOPE_LOCK(m_blockListMutex);
        if (m_blockList.size() > 0)
        {
            blockList.splice(blockList.end(), m_blockList);
        }
    }
    //saveBlock(blockList);

    PACKET_BLOCK_LIST::iterator it = blockList.begin();
    for (; it != blockList.end(); ++it)
    {
        DataBlock_t* block = *it;
        if (m_captureState != CAPTURE_STARTED)
        {
            // Release everything and bail out
            releaseBlockResource(block);
            continue;
        }

        switchTarget();

        if (-2 == saveBlock(block)) // stop because of wrap
        {
            stopCapture();
        }
    }

    return blockList.size();
}

int CBlockCaptureImpl::finalizeBlock(DataBlock_t* _block, bool _ok)
{
    assert(_block);
    OutRes_t& outRes = m_outRes[_block->dataDesc.resIndex];

    m_saveByteCount += _block->dataDesc.blockSize;
    m_savePktCount += _block->dataDesc.pktCount;

    if (_block->dataDesc.pktCount == 0)    // padding block
    {
        outRes.blkInfoDB.flush();
        return 0;
    }

    //
    // TODO: Maybe need to flush outRes.blkInfoDB timely
    //
    if (g_env->m_config.debugMode >= DEBUG_BLOCK)
    {
        CBlockCaptureImpl* capture = this;
        DataBlock_t* block = _block;
        LOG_PERFORMANCE_DATA();
    }

    ++m_finishWriteCount;

    //
    // insert the packet meta information into db
    //
    if (g_env->m_config.engine.enableParsePacket)
    {
        recyclePktMeta(_block, _ok);
    }

    //
    // last block of a file, do once commit
    //
    if (_ok) addBlockRecord(_block);

    if (_block->dataDesc.offset + _block->dataDesc.usedSize >= g_env->m_config.storage.fileSize) // the padding block
    {
        outRes.blkInfoDB.flush();
    }

    g_env->m_capTimeRange.startTime = MyMin(g_env->m_capTimeRange.startTime, _block->dataDesc.firstPacketTime);
    g_env->m_capTimeRange.stopTime  = MyMax(g_env->m_capTimeRange.stopTime,  _block->dataDesc.lastPacketTime);

    releaseBlockResource(_block);

    //
    // flush the block information every 5 sec
    //
    static time_t   lastUpdateTime = time(NULL);
    time_t now = time(NULL);
    if (now - lastUpdateTime >= 5)
    {
        lastUpdateTime = now;
        flushDB();
    }

    return 0;
}

void CBlockCaptureImpl::recyclePktMeta(DataBlock_t *_block, bool _writeSucceed)
{
    u_int32 idx = _block->dataDesc.metaBlkStartIdx;
    u_int32 stopIdx = _block->dataDesc.metaBlkStopIdx;
    if (idx > stopIdx)  stopIdx += m_packetRing->GetMetaBlockCount();

    for (; idx <= stopIdx; ++idx)
    {
        PktMetaBlk_t *metaBlk = m_packetRing->GetMetaBlkByIdx(idx);
        if (metaBlk->dataBlk != _block)
            continue; // Multi-threads capturing might have discontinuous meta block

        releasePktsInBlk(metaBlk, _writeSucceed);
        m_packetRing->FreeFullMetaBlk(metaBlk);
    }
}

void CBlockCaptureImpl::releasePktsInBlk(PktMetaBlk_t *_metaBlk, bool _writeSucceed)
{
    // Get every packet meta block and insert every meta to DB.
    DataBlock_t *dataBlock = _metaBlk->dataBlk;
    u_int64 blkDataOffset = dataBlock->dataDesc.type == PACKET_PCAP
        ? (u_int64)dataBlock->dataDesc.pData - (u_int64)m_modInfo.blockViewStart
        : (u_int64)dataBlock->dataDesc.pData;
    for (u_int32 cursor = 0; cursor < _metaBlk->size; ++cursor)
    {
        PacketMeta_t* metaInfo = m_packetRing->GetPktMetaInBlk(_metaBlk, cursor);

        if (g_env->m_config.engine.enableInsertDB && _writeSucceed)
        {
            packet_header_t header;

            // get the packet offset in the file
            u_int32 offsetInBlk = metaInfo->headerAddr - blkDataOffset;
            assert(offsetInBlk < ONE_MB);
            assert(metaInfo->headerAddr >= blkDataOffset);

            metaInfo->basicAttr.offset = offsetInBlk + dataBlock->dataDesc.offset;

            assert(metaInfo->basicAttr.offset >= sizeof(pcap_file_header));
            assert(metaInfo->basicAttr.offset < g_env->m_config.storage.fileSize);

#if defined(DEBUG) || defined(_DEBUG)
            if (_metaBlk->blkPos == dataBlock->dataDesc.metaBlkStartIdx && cursor == 0)
                assert(metaInfo->basicAttr.ts == dataBlock->dataDesc.firstPacketTime);

            bool isLastPkt = _metaBlk->isFinal && (cursor == _metaBlk->size - 1);
            if (!isLastPkt)    // not the last packet
            {
                packet_header_t* pktHeader = m_packetRing->GetPacketHeader(_metaBlk->type, metaInfo, &header);
                assert(metaInfo->basicAttr.ts == ((u_int64)pktHeader->ts.tv_sec * NS_PER_SECOND + pktHeader->ts.tv_nsec));
                assert(metaInfo->basicAttr.pktLen > 1);
            }
#endif
            if (_metaBlk->type == PACKET_PCAP)
            {
                packet_header_t* pktHeader = m_packetRing->GetPacketHeader(_metaBlk->type, metaInfo, &header);
                if (pktHeader)
                    metaInfo->basicAttr.pktLen = pktHeader->caplen + sizeof(packet_header_t);
            }
            addPacketRecord(metaInfo, dataBlock->dataDesc.resIndex);
        }

        releasePacketResource(_metaBlk->type, metaInfo);
    }
}

int CBlockCaptureImpl::OnDataWritten(void* _handler, PPER_FILEIO_INFO_t _ioContext, DWORD _bytesCount)
{
    assert(!((_ioContext->context == NULL) ^ (_ioContext->context2 == NULL)));

    CBlockCaptureImpl* capture = (CBlockCaptureImpl*)_ioContext->context;
    if (capture)    // when null, this is the file header block
    {
        DataBlock_t* block = (DataBlock_t*)_ioContext->context2;
        capture->finalizeBlock(block, true);
    }

    return 0;   // the _ioContext should be released by framework
}

int CBlockCaptureImpl::OnDataWrittenError(void* _handler, PPER_FILEIO_INFO_t _ioContext, int _err)
{
    assert(!((_ioContext->context == NULL) ^ (_ioContext->context2 == NULL)));

    CBlockCaptureImpl* capture = (CBlockCaptureImpl*)_ioContext->context;
    if (capture)
    {
        DataBlock_t* block = (DataBlock_t*)_ioContext->context2;

        RM_LOG_ERROR("Fail to write block to disk at @" << block << ", " << block->dataDesc.usedSize);
        capture->finalizeBlock(block, false);
    }

    return 0;
}

void CBlockCaptureImpl::releasePacketResource(PacketType_e type, PacketMeta_t* _metaInfo)
{
    if (!_metaInfo) return;
}

void CBlockCaptureImpl::releaseBlockResource(DataBlock_t* _block)
{
    assert(_block);
    if (!_block)    return;
    assert(_block->dataDesc.capObject);

    ((CBlockCapture*)_block->dataDesc.capObject)->releaseBlock(_block);
}

int CBlockCaptureImpl::updateStatistic(time_t _now, const char* _step)
{
    static time_t prevTime = 0;
    static u_int64 prevPktCount = 0;
    static u_int64 prevByteCount = 0;
    static u_int64 tick = 0;

    Statistic_t summary = { 0 };
    getStatistic(summary);

    int deltaSec = prevTime != 0 ? _now - prevTime : 1;
    prevTime = _now;
    if (deltaSec == 0)  return 0;

    u_int64 deltaBytes = (summary.bytesSaveCount - prevByteCount) / deltaSec;
    u_int64 deltaPkts = (summary.pktSaveCount - prevPktCount) / deltaSec;
    tm* ltm = localtime(&_now);
    summary.utilization = deltaBytes * 800.0 / 10000 / ONE_MILLION;

    CStdString strUtil;
    strUtil.Format("%.2f", summary.utilization);
    stringstream fmtString;
    fmtString << _step << ltm->tm_hour << ":" << ltm->tm_min << ":" << ltm->tm_sec \
        << " Pkts= " << summary.pktSeenByNIC << ", " << summary.pktRecvCount << ", " << summary.pktSaveCount << "; " \
        << LiangZhu::ChangeUnit(summary.bytesSaveCount, 1024, "B").c_str() << "; "    \
        << "Speed= " << LiangZhu::ChangeUnit(deltaBytes, 1024, "BPS", 2).c_str() << ", " \
        << LiangZhu::ChangeUnit(deltaPkts, 1000, "pps").c_str() << "; ";

    if (m_packetRing && m_packetRing->IsReady())
    {
        vector<u_int32> vec;
        m_packetRing->GetAllModulePosInfo(vec);
        fmtString << vec[0] << ", " << vec[vec.size() - 1] << ", ";
    }
    fmtString << "Drop=" << summary.pktDropCount << "\r";
    cerr << fmtString.str();

    prevPktCount = summary.pktSaveCount;
    prevByteCount = summary.bytesSaveCount;

    if (++tick % 300 == 0)   // every 15 min
    {
        RM_LOG_INFO_S(fmtString.str());
    }

    return 0;
}
