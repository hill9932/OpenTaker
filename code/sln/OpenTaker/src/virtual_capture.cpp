#include "virtual_capture.h"
#include "capture_store.h"
#include "file_system.h"
#include "global.h"
#ifdef WIN32
#include <malloc.h>
#endif


CVirtualCapture::CVirtualCapture()
{
    bzero(m_srcBlocks, SOURCE_BLOCK_COUNT * sizeof(VirtualBlock_t));
    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
        m_captureBlockPool[i].resetMem(SOURCE_BLOCK_COUNT);
}

CVirtualCapture::~CVirtualCapture()
{

}

int CVirtualCapture::scanLocalNICs()
{
    int curSize = m_localNICs.size();
    int devSize = 0;

    // check whether has detected
    for (unsigned int i = 0; i < m_localNICs.size(); ++i)
    {
        if (m_localNICs[i].type == DEVICE_TYPE_VIRTUAL)
            ++devSize;
    }

    if (devSize != 0)   return devSize;
    int portIndex = 0;
    if (curSize > 0)
        portIndex = m_localNICs[curSize - 1].portIndex + m_localNICs[curSize - 1].portCount;

    DeviceST dev;
    dev.name = "Capture packets from pcap file";
    dev.desc = "Virtual Capture Device V1.0";

    dev.portStat[0].status = LINK_ON;
    dev.portCount = 1;
    dev.portIndex = portIndex;
    dev.myDevIndex = 0;
    dev.myPortIndex = 0;
    dev.linkSpeed = 10000;  // 10 Gbps
    dev.type = DEVICE_TYPE_VIRTUAL;

    m_localNICs.push_back(dev);

    return m_localNICs.size() - curSize;
}

int CVirtualCapture::getStatistic(Statistic_t& _pcapStat)
{
    for (unsigned int i = 0; i < m_device.portCount; ++i)
    {
        Statistic_t portStat = { 0 };
        if (0 == getStatistic(portStat, i))
        {
            _pcapStat.pktSeenByNIC += portStat.pktSeenByNIC;
            _pcapStat.bytesSeenByNIC += portStat.bytesSeenByNIC;
            _pcapStat.pktDropCount += portStat.pktDropCount;
            _pcapStat.pktErrCount += portStat.pktErrCount;
            _pcapStat.rmonStat.brdcst_pkts += portStat.rmonStat.brdcst_pkts;
            _pcapStat.rmonStat.mcst_pkts += portStat.rmonStat.mcst_pkts;
        }
    }

    _pcapStat.index = m_devIndex;
    _pcapStat.port = -1;
    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        _pcapStat.pktRecvCount += m_pktRecvCount[i];
        _pcapStat.bytesRecvCount += m_byteRecvCount[i];
    }

    return 0;
}

int CVirtualCapture::getStatistic(Statistic_t& _pcapStat, int _port)
{
    bzero(&_pcapStat, sizeof(_pcapStat));

    int i = _port;
    _pcapStat.rmonStat.total_pkts = m_rmonStats[i].total_pkts;
    _pcapStat.rmonStat.total_bytes = m_rmonStats[i].total_bytes;
    _pcapStat.rmonStat.err_pkts = m_rmonStats[i].err_pkts;
    _pcapStat.rmonStat.drop_pkts = m_rmonStats[i].drop_pkts;
    _pcapStat.rmonStat.brdcst_pkts = m_rmonStats[i].brdcst_pkts;
    _pcapStat.rmonStat.mcst_pkts = m_rmonStats[i].mcst_pkts;
    _pcapStat.rmonStat.pkts_lt_64 = m_rmonStats[i].pkts_lt_64;
    _pcapStat.rmonStat.pkts_eq_64 = m_rmonStats[i].pkts_eq_64;
    _pcapStat.rmonStat.pkts_65_127 = m_rmonStats[i].pkts_65_127;
    _pcapStat.rmonStat.pkts_128_255 = m_rmonStats[i].pkts_128_255;
    _pcapStat.rmonStat.pkts_256_511 = m_rmonStats[i].pkts_256_511;
    _pcapStat.rmonStat.pkts_512_1023 = m_rmonStats[i].pkts_512_1023;
    _pcapStat.rmonStat.pkts_1024_1518 = m_rmonStats[i].pkts_1024_1518;
    _pcapStat.rmonStat.pkts_1519_2047 = m_rmonStats[i].pkts_1519_2047;
    _pcapStat.rmonStat.pkts_2048_4095 = m_rmonStats[i].pkts_2048_4095;
    _pcapStat.rmonStat.pkts_4096_8191 = m_rmonStats[i].pkts_4096_8191;
    _pcapStat.rmonStat.pkts_8192_9018 = m_rmonStats[i].pkts_8192_9018;
    _pcapStat.rmonStat.pkts_9019_9198 = m_rmonStats[i].pkts_9019_9198;
    _pcapStat.rmonStat.pkts_oversize = m_rmonStats[i].pkts_oversize;

    _pcapStat.pktSeenByNIC = m_pktRecvCount[i];
    _pcapStat.bytesSeenByNIC = m_byteRecvCount[i];

    _pcapStat.index = m_devIndex;
    _pcapStat.port = _port;
    return 0;
}

bool CVirtualCapture::openDevice_(int _index, const char* _devName)
{
    CStdString filePath = g_env->m_config.engine.pcapFile;
    if (filePath.IsEmpty())
        filePath = GetAppDir() + "virtual/validation.pcap";

    if (!IsFileExist(filePath) || 0 != m_pcapFile.open(filePath, ACCESS_READ, FILE_OPEN_EXISTING, false, false))
    {
        RM_LOG_ERROR("Fail to open: " << filePath);
        return false;
    }

    RM_LOG_INFO("Success to open: " << filePath);

    if (g_env->m_config.testMetaBlock == 1) {
        if (0 != fillBlockPool()) {
            RM_LOG_ERROR("Failed to init block pool for test.");
            return false;
        }
    }

    return true;
}

bool CVirtualCapture::closeDevice_()
{
    if (!isOpen())  return true;
    return 0 == m_pcapFile.close();
}

int CVirtualCapture::prepareResource()
{
    return 0;
}

int CVirtualCapture::releaseResource()
{
    return 0;
}

int CVirtualCapture::startCapture_(void* _arg)
{
    CaptureContext_t* context = (CaptureContext_t*)_arg;
    RM_LOG_INFO("Start capturing port " << context->portId);

    struct timeval tsVal;
    u_int64 ts = 0;
    while (g_env->enable())
    {
        if (m_captureState[context->captureId] == CAPTURE_STOPPING)
        {
            CBlockCapture::handlePacket(context->captureId, m_device.portIndex, NULL, NULL, true);
            return 1;
        }

        // Wait until all threads are ready, current capture thread and store
        // thread, maybe process thread, too.
        if (!isCaptureStarted(context->captureId))
        {
            SleepSec(1);
            continue;
        }

        if (g_env->m_config.testMetaBlock == 1)
        {
            int r = handlePacket(context->captureId);
            if (r < 0) YieldCurThread();

            continue;
        }

        u_int64 fileSize = m_pcapFile.getFileSize();
        byte* fileData = m_pcapFile.getBuf();
        if (!fileData)  return -1;

        u_int64 offset = sizeof(pcap_file_header);
        int pktLen = 0;
        packet_header_t* pktHeader = NULL;
        byte *pktData = NULL;

        if (0 == ts)
        {
            gettimeofday(&tsVal, NULL);
            ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;
        }
        while (offset < fileSize)
        {
            gettimeofday(&tsVal, NULL);
            ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;

            ts += 2;
            pktHeader = (packet_header_t*)(fileData + offset);
            pktLen = sizeof(packet_header_t) + pktHeader->caplen;
            pktData = (byte*)pktHeader + sizeof(packet_header_t);
            offset += pktLen;

//            assert(pktLen == 76 || pktLen == 488);

            packet_header_t newPktHeader = *pktHeader;
            newPktHeader.ts.tv_sec = ts / NS_PER_SECOND;
            newPktHeader.ts.tv_nsec = ts % NS_PER_SECOND;

            if (g_env->m_config.justTestCapture > 0)
                testCapture(context->captureId, m_device.portIndex, &newPktHeader, pktData, true);
            else
                CBlockCapture::handlePacket(context->captureId,
                                            m_device.portIndex,
                                            &newPktHeader, pktData, true);
            SleepUS(1);

            if (m_captureState[context->captureId] == CAPTURE_STOPPING)
                break;

            if (!g_env->enable())   return 1;
        }

        //break;
    }

    RM_LOG_INFO("CVirtualCapture Capturing exited.");

    return 1;
}

int CVirtualCapture::setCaptureFilter_(const shared::FilterConfig& _filter, int _port)
{
    return 0;
}

int CVirtualCapture::setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter)
{
    return -1;
}

int CVirtualCapture::fillBlockPool()
{
    u_int32 i = 0;
    VirtualBlock_t *dataBlock = &m_srcBlocks[i];
#ifdef WIN32
    dataBlock->pData = (byte*)_aligned_malloc(ONE_MB, SECTOR_ALIGNMENT);
#else
    dataBlock->pData = (byte*)aligned_alloc(SECTOR_ALIGNMENT, ONE_MB);
#endif
    if (dataBlock->pData == NULL)
        return -1;

    while (i < SOURCE_BLOCK_COUNT) {
        u_int64 fileSize = m_pcapFile.getFileSize();
        byte* fileData = m_pcapFile.getBuf();
        if (!fileData)  return -1;

        u_int64 offset = sizeof(pcap_file_header);
        int pktLen = 0;
        packet_header_t* pktHeader = NULL;
        byte *pktData = NULL;

        struct timeval tsVal;
        gettimeofday(&tsVal, NULL);
        u_int64 ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;

        while (offset < fileSize && i < SOURCE_BLOCK_COUNT) {
            gettimeofday(&tsVal, NULL);
            ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;

            ts += 2;
            pktHeader = (packet_header_t*)(fileData + offset);
            pktLen = sizeof(packet_header_t) + pktHeader->caplen;
            pktData = (byte*)pktHeader + sizeof(packet_header_t);
            offset += pktLen;

            packet_header_t newPktHeader = *pktHeader;
            newPktHeader.ts.tv_sec = ts / NS_PER_SECOND;
            newPktHeader.ts.tv_nsec = ts % NS_PER_SECOND;

            if (dataBlock->dataDesc.usedSize + pktLen > DATA_BLOCK_SIZE)
            {
                int tail = dataBlock->dataDesc.usedSize % SECTOR_ALIGNMENT;
                u_int32 padSize = (tail == 0 ? 0 : (SECTOR_ALIGNMENT - tail));
                if (g_env->m_config.engine.isRealTime &&
                    g_env->m_config.engine.isSecAlign && 
                    padSize != 0)
                {
                    // only real time and need sector align
                    if (padSize > sizeof(packet_header_t))
                    {
                        // add the fake packet in the end
                        padding(dataBlock->pData + dataBlock->dataDesc.usedSize, padSize);
                    }
                    dataBlock->dataDesc.usedSize += padSize;
                }
                m_srcBlkQueue.push(dataBlock);

                if (++i >= SOURCE_BLOCK_COUNT)
                {
                    break;
                }
                dataBlock = &m_srcBlocks[i];
#ifdef WIN32
                dataBlock->pData = (byte*)_aligned_malloc(ONE_MB, SECTOR_ALIGNMENT);
#else
                dataBlock->pData = (byte*)aligned_alloc(SECTOR_ALIGNMENT, ONE_MB);
#endif
                if (dataBlock->pData == NULL)
                    return -1;
            }

            byte* dst = dataBlock->pData + dataBlock->dataDesc.usedSize;
            memcpy(dst, &newPktHeader, sizeof(packet_header_t));
            dst += sizeof(packet_header_t);
            memcpy(dst, pktData, newPktHeader.caplen);
            dataBlock->dataDesc.usedSize += pktLen;
            dataBlock->dataDesc.pktCount ++;
        }
    }

    return 0;
}

int CVirtualCapture::handlePacket(int _captureId)
{
    DataBlock_t *srcBlock = NULL;
    if (!m_srcBlkQueue.pop(srcBlock) || srcBlock == NULL) {
        return -1;
    }

    u_int64 offset = 0;
    VirtualBlock_t* dataBlock = m_captureBlockPool[_captureId].getBlock();
    bzero(&dataBlock->dataDesc, sizeof(DataBlockDesc_t));
    dataBlock->dataDesc.type = PACKET_N_BLOCK;

    struct timeval tsVal;
    gettimeofday(&tsVal, NULL);
    u_int64 ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;
    dataBlock->dataDesc.firstPacketTime = ts;

#ifdef TEST_MUITI_FLOW
    static u_int32 s_blks = 0;
    ++s_blks;
#endif

    PacketMeta_t* metaInfo = NULL;
    if (g_env->m_config.engine.enableParsePacket) {
        u_int32 pktCount = 0;
        while (pktCount <= srcBlock->dataDesc.pktCount) {
            packet_header_t* pktHeader = (packet_header_t*)(srcBlock->dataDesc.pData + offset);
            if (pktHeader->ts.tv_sec == 0) {
                pktCount++;
                continue;
            }

            pktHeader->ts.tv_sec = ts / NS_PER_SECOND;
            pktHeader->ts.tv_nsec = ts % NS_PER_SECOND;

            metaInfo = m_myImpl->requestMeta4PktBlk(_captureId, (DataBlock_t*)dataBlock);
            metaInfo->headerAddr = (u_int64)pktHeader; // point to the header
#ifdef TEST_MUITI_FLOW
            metaInfo->testNum = s_blks / 10;
#endif
            metaInfo->indexValue.ts = ts;
            metaInfo->indexValue.portNum = m_device.portIndex;

            metaInfo->indexValue.pktLen = pktHeader->caplen;
            metaInfo->indexValue.wireLen = pktHeader->len;

            //byte *pktData = (byte*)pktHeader + sizeof(packet_header_t);
            //ProcessPacket2(pktData, metaInfo);

            if (pktCount++ == 0) {
                dataBlock->dataDesc.metaBlkStartIdx =
                    m_myImpl->getCurMetaBlkPos(_captureId);
            }

            offset += sizeof(packet_header_t) + pktHeader->caplen;
            ts += 2;
        }

        dataBlock->dataDesc.metaBlkStopIdx = m_myImpl->getCurMetaBlkPos(_captureId);
    }

    /**
     * Output statistics information
     */
    m_pktRecvCount[_captureId] += srcBlock->dataDesc.pktCount;
    m_byteRecvCount[_captureId] += srcBlock->dataDesc.usedSize;

    dataBlock->dataDesc.lastPacketTime = ts;
    dataBlock->dataDesc.pData = srcBlock->dataDesc.pData;
    dataBlock->dataDesc.usedSize = srcBlock->dataDesc.usedSize;
    dataBlock->dataDesc.pktCount = srcBlock->dataDesc.pktCount;
    dataBlock->dataDesc.capObject = this;
    dataBlock->dataDesc.param1 = (void*)(u_int64)_captureId;
    dataBlock->dataDesc.param2 = (void*)srcBlock;

    handleBlock((DataBlock_t*)dataBlock);

    return 0;
}

int CVirtualCapture::releaseBlock(DataBlock_t* _block)
{
    VirtualBlock_t *virtBlock = (VirtualBlock_t *)_block;
    VirtualBlock_t *src = (VirtualBlock_t *)virtBlock->dataDesc.param2;
    assert(src);
    if (!m_srcBlkQueue.push(src)) {
        assert(false);
    }

    int captureId = (int)(u_int64)virtBlock->dataDesc.param1;
    m_captureBlockPool[captureId].release(virtBlock);
    return 0;
}
