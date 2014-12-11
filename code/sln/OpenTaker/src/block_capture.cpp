#include "block_capture.h"
#include "capture_store.h"
#include "global.h"
#include "file_system.h"
#include "system_.h"
#include "net_util.h"
#include "decode_basic.h"
#include "pcap_file.h"

CBlockCapture::CBlockCapture()
{
    m_myImpl        = NULL;
    bzero(&m_captureThread, sizeof(m_captureThread));
    bzero(&m_bufBlock, sizeof(m_bufBlock));
    bzero(&m_prevPacket, sizeof(m_prevPacket));
    bzero(&m_portStats, sizeof(m_portStats));

    m_capThreadCount = 0;
    m_isReady = false;

    CreateAllDir(CStdString("./PerfTest"));
}

CBlockCapture::~CBlockCapture()
{
    stopCapture();
}

bool CBlockCapture::openDevice(int _index, const char* _devName)
{
    if (isOpen())   return true;
    if (openDevice_(_index, _devName))
    {
        if (!m_myImpl)
            m_myImpl = CBlockCaptureImpl::GetInstance();

        m_myImpl->regObj(this);
        return true;
    }

    return false;
}

//bool CBlockCapture::openDevice_(int _index, const char* _devName) = 0;

int CBlockCapture::startCapture(CaptureConfig_t& _config)
{
    RM_LOG_INFO("Prepare to start capture: " << m_device.name);

    if (getCaptureState() != CAPTURE_STOPPED)
    {
        RM_LOG_ERROR("Device is capturing.");
        return 0;
    }

    m_capThreadCount = MyMin((u_int32)MAX_CAPTURE_THREAD, m_device.portCount);
    if (0 != prepareResource(_config))
    {
        RM_LOG_ERROR("Fail to prepare resource.");
        return -1;
    }
    RM_LOG_INFO("Launching " << m_capThreadCount << " capture thread.");

    m_capThreadCount = 0;
    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        if (i >= (int)m_device.portCount) break;

        if (m_captureState[i] == CAPTURE_STARTED)    continue;
        m_captureState[i] = CAPTURE_STARTING;

        bzero(&m_pktRecvCount[i], sizeof(m_pktRecvCount[i]));
        bzero(&m_byteRecvCount[i], sizeof(m_byteRecvCount[i]));

        if (!m_captureThread[i])
        {
            CaptureContext_t* context = new CaptureContext_t;
            context->captureObj = this;
            context->portId = i;
            context->captureId = i;
            m_captureThread[i] = new CSimpleThread(doTheCapture_, context);
        }

        m_captureState[i] = CAPTURE_STARTED;
        m_capThreadCount++;
    }

    m_isReady = true;
    RM_LOG_INFO("Success to start capture[" << m_devIndex << "]: " << m_device.name);
    return 0;
}

int CBlockCapture::stopCapture()
{
    if (getCaptureState() != CAPTURE_STARTED)
    {
        return 0;
    }

    RM_LOG_INFO("Prepare to stop capture: " << m_device.name);

    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        if (m_captureState[i] == CAPTURE_STOPPED)
        {
            SAFE_DELETE(m_captureThread[i]);
            continue;
        }

        m_captureState[i] = CAPTURE_STOPPING;

        while (m_captureState[i] != CAPTURE_STOPPED &&
               m_captureState[i] != CAPTURE_ERROR)
            SleepSec(1);

        SAFE_DELETE(m_captureThread[i]);
    }
    
    releaseResource();
    m_isReady = false;

    RM_LOG_INFO("Success to stop capture " << m_device.name);

    return 0;
}

int CBlockCapture::prepareResource(CaptureConfig_t& _config)
{
    return 0;
}

int CBlockCapture::releaseResource()
{
    return 0;
}

int CBlockCapture::setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter)
{
    return 0;
}

ICaptureFile* CBlockCapture::createCaptureFile()
{
    return new CPCAPFile;
}

int CBlockCapture::startCapture_(void* _arg)
{
    return 0;
}

int CBlockCapture::closeDevice()
{
    SCOPE_LOCK(m_deviceLock);
    if (closeDevice_())
    {
        m_myImpl->unregObj(this);
        return 0;
    }

    return -1;
}

int CBlockCapture::doTheCapture_(void* _obj)
{
    if (!_obj)  return -1;

    int z = 0;
    CaptureContext_t* context = (CaptureContext_t*)_obj;
    CBlockCapture *captureObj = context->captureObj;

    z = captureObj->startCapture_(_obj); 

    captureObj->m_captureState[context->captureId] = CAPTURE_STOPPED;
    RM_LOG_INFO(captureObj->m_device.name << " capture " << context->captureId << " exited.");

    return z;
}

int CBlockCapture::handlePacket(int _captureId, int _portIndex, packet_header_t* _header, const byte* _data, bool _rmon)
{
    assert(_captureId < MAX_CAPTURE_THREAD);

    bool flush = false;
    if (!_header)  flush = true;     // just flush, happened when capture timeout

    DataBlock_t* dataBlock = m_bufBlock[_captureId];
    if (flush && (!dataBlock || dataBlock->dataDesc.usedSize < 80))
        return -1;
    if (flush || (dataBlock != NULL && isDataBlockFull(_captureId, _header)))
    {
        flushDataBlock(_captureId);
        if (flush) return 0;
    }

    if (m_prevPacket[_captureId])
    {
        u_int64 pktTS = (u_int64)_header->ts.tv_sec * NS_PER_SECOND + _header->ts.tv_nsec;
        u_int64 lastPktTS = (u_int64)m_prevPacket[_captureId]->ts.tv_sec * NS_PER_SECOND + m_prevPacket[_captureId]->ts.tv_nsec;
        if (pktTS < lastPktTS)
        {
            RM_LOG_ERROR("Current TS = " << pktTS << ", last TS = " << lastPktTS);
//            assert(false);
        }
    }

    //
    // copy the packet to the block memory
    //
    if (_header)
    {
        PacketMeta_t *metaInfo = copy2Block(_captureId, _portIndex, _header, _data);
        //
        // calculate rmon values
        //
        if (_rmon && metaInfo)
        {
            calcRmon(_portIndex, _header, metaInfo, _data);
        }
    }

    return 0;
}

bool CBlockCapture::isDataBlockFull(int _captureId, packet_header_t *_header)
{
    if (!_header) return false;

    DataBlock_t* dataBlock = m_bufBlock[_captureId];
    if (!dataBlock) return false;

    u_int32 saveLen = sizeof(packet_header_t) +
        MyMin(_header->caplen, g_env->m_config.storage.sliceSize);
    return DATA_BLOCK_SIZE - dataBlock->dataDesc.usedSize < saveLen;
}

void CBlockCapture::flushDataBlock(int _captureId)
{
    DataBlock_t *dataBlock = m_bufBlock[_captureId];
    assert(dataBlock);
    //
    // padding the block to make it sector size align if needed
    //
    alignBlock(_captureId, dataBlock);

    if (g_env->m_config.engine.enableParsePacket)
    {
        dataBlock->dataDesc.metaBlkStopIdx = m_myImpl->getCurMetaBlkPos(_captureId);
    }

    if (g_env->m_config.engine.isSecAlign)
        assert(dataBlock->dataDesc.blockSize % SECTOR_ALIGNMENT == 0);

    handleBlock(dataBlock);
    
    /**
     * Set flushed data block to NULL
     */
    m_bufBlock[_captureId] = NULL;
}

void CBlockCapture::alignBlock(int _captureId, DataBlock_t *_dataBlock)
{
    int tail = _dataBlock->dataDesc.usedSize % SECTOR_ALIGNMENT;
    u_int32 padSize = tail == 0 ? 0 : SECTOR_ALIGNMENT - tail;
    if (g_env->m_config.engine.isSecAlign &&
        padSize != 0)
    {
        // only real time and need sector align
        if (padSize > sizeof(packet_header_t))
        {
            padding(_dataBlock, padSize); // add the fake packet in the end
        }
        else
        {
            m_prevPacket[_captureId]->caplen += padSize; // adjust the last packet's length,
            m_prevPacket[_captureId]->len += padSize;    // TODO: check this
            _dataBlock->dataDesc.usedSize += padSize;
        }
    }
}

PacketMeta_t* CBlockCapture::copy2Block(int _captureId, int _portIndex, packet_header_t* _header, const byte* _data)
{
    assert(_header);

    static PacketMeta_t pktInfo;
    PacketMeta_t *metaInfo = NULL;
    DataBlock_t *&dataBlock = m_bufBlock[_captureId];
    if (!dataBlock || isDataBlockFull(_captureId, _header))
    {
        /**
         * If this data block is full, it must have been flushed, switch to
         * another one.
         */
        dataBlock = m_myImpl->requestBlock(_captureId);
        if (!dataBlock) return NULL;

        if (g_env->m_config.engine.enableParsePacket)
        {
            metaInfo = m_myImpl->requestMeta4PktBlk(_captureId, dataBlock);
            dataBlock->dataDesc.metaBlkStartIdx = m_myImpl->getCurMetaBlkPos(_captureId);
        }
        else
            metaInfo = &pktInfo;
    }

    /**
     * if parse packet is disable, use this local variable to process the packet
     */
    if (metaInfo == NULL)
    {
        if (g_env->m_config.engine.enableParsePacket)
            metaInfo = m_myImpl->requestMeta4PktBlk(_captureId, dataBlock);
        else
            metaInfo = &pktInfo;
    }

    _header->caplen = MyMin(_header->caplen, g_env->m_config.storage.sliceSize);
    u_int64 pktTS = (u_int64)_header->ts.tv_sec * NS_PER_SECOND + _header->ts.tv_nsec;
    u_int32 saveLen = _header->caplen + sizeof(packet_header_t);

    byte* dst = dataBlock->data + dataBlock->dataDesc.usedSize;
    m_prevPacket[_captureId] = (packet_header_t*)dst;

    memcpy(dst, _header, sizeof(packet_header_t));
    dst += sizeof(packet_header_t);
    memcpy(dst, _data, _header->caplen);
    dataBlock->dataDesc.usedSize += saveLen;
    dataBlock->dataDesc.pktCount++;
    dataBlock->dataDesc.lastPacketTime = pktTS;

    if (dataBlock->dataDesc.firstPacketTime == 0)
    {
        dataBlock->dataDesc.firstPacketTime = pktTS;
    }

    metaInfo->basicAttr.ts = (u_int64)_header->ts.tv_sec * NS_PER_SECOND + _header->ts.tv_nsec;
    metaInfo->basicAttr.portNum = _portIndex;
    metaInfo->headerAddr = (u_int64)m_prevPacket[_captureId] - (u_int64)m_myImpl->m_modInfo.blockViewStart;      // point to the header

    ProcessPacketBasic((byte*)_data, metaInfo);

#if defined(DEBUG) || defined(_DEBUG)
    if (g_env->m_config.debugMode >= DEBUG_PACKET)
    {
        m_fsCapturePacket[_captureId] << pktTS << ", " << endl;
    }
#endif

    metaInfo->basicAttr.wireLen = _header->len;
    metaInfo->basicAttr.pktLen = _header->caplen;  // the packet is ready

    if (_header->ts.tv_sec != 0)
    {
        ++m_pktRecvCount[_captureId];
        m_byteRecvCount[_captureId] += _header->caplen;     // this value belongs to CBlockCapture object, CBlockCaptureImpl will not call this function
    }

    return metaInfo;
}

int CBlockCapture::calcRmon(int _portIndex, const packet_header_t* _header, PacketMeta_t* _metaInfo, const byte* _pktData)
{
    if (m_devIndex < 0) return -1;

    int myPortIndex = _portIndex - m_device.portIndex;
    assert(myPortIndex < MAX_PORT_NUM);

    m_portStats[myPortIndex].rmonStat.total_pkts++;
    m_portStats[myPortIndex].rmonStat.total_bytes += _header->caplen;

    if (ChangQian::IsMcstMacAddress(_pktData))
    {
        if (ChangQian::IsBcstMacAddress(_pktData))
            m_portStats[myPortIndex].rmonStat.brdcst_pkts++;
        else
            m_portStats[myPortIndex].rmonStat.mcst_pkts++;
    }

    if (_header->len < 64)
        m_portStats[myPortIndex].rmonStat.pkts_lt_64++;
    else if (_header->len == 64)
        m_portStats[myPortIndex].rmonStat.pkts_eq_64++;
    else if (_header->len < 128)
        m_portStats[myPortIndex].rmonStat.pkts_65_127++;
    else if (_header->len < 256)
        m_portStats[myPortIndex].rmonStat.pkts_128_255++;
    else if (_header->len < 512)
        m_portStats[myPortIndex].rmonStat.pkts_256_511++;
    else if (_header->len < 1024)
        m_portStats[myPortIndex].rmonStat.pkts_512_1023++;
    else if (_header->len < 1519)
        m_portStats[myPortIndex].rmonStat.pkts_1024_1518++;
    else if (_header->len < 2048)
        m_portStats[myPortIndex].rmonStat.pkts_1519_2047++;
    else if (_header->len < 4096)
        m_portStats[myPortIndex].rmonStat.pkts_2048_4095++;
    else if (_header->len < 8192)
        m_portStats[myPortIndex].rmonStat.pkts_4096_8191++;
    else if (_header->len < 9019)
        m_portStats[myPortIndex].rmonStat.pkts_8192_9018++;
    else if (_header->len < 10240)
        m_portStats[myPortIndex].rmonStat.pkts_ge_9199++;
    else
        m_portStats[myPortIndex].rmonStat.pkts_oversize++;

    return 0;
}

int CBlockCapture::handleBlock(DataBlock_t* _block, int _blockCount)
{
    if (!_block)    return -1;

    if (g_env->m_config.debugMode >= DEBUG_BLOCK)
    {
        struct timeval ts;
        gettimeofday(&ts, NULL);
        CStdString strIndex;
        //if (g_env->m_config.engine.debugMode == DEBUG_PACKET) strIndex = getBlockMetaIndex(_block);

        SCOPE_LOCK(m_logMutex);
        m_fsCaptureBlock << ts.tv_sec << "." << ts.tv_usec << ": " \
            << _block->dataDesc.id << ", " \
            << (void*)_block << ", " \
            << _block->dataDesc.usedSize << ", " \
            << _block->dataDesc.firstPacketTime << endl;
    }

    int z = 0;
    if (!g_env->m_config.engine.enableParsePacket)
    {
        z = m_myImpl->blockArrived(_block);     // queue to block list waiting to save
    }
    else
    {
        m_myImpl->finalMetaBlk(_block->dataDesc.captureId);
    }

    return z;
}


int CBlockCapture::detectFinishIo(int _captureId)
{
    int reqNum = 0;
    vector<PPER_FILEIO_INFO_t> vec;
    int n = m_myImpl->getFinishedRequest(vec);
    if (n > 0)
    {
        for (int i = 0; i < n; ++i)
        {
            PPER_FILEIO_INFO_t ioContext = vec[i];
            DataBlock_t* block = (DataBlock_t*)ioContext->context2;
            if (block &&
                block->dataDesc.capObject == this &&
                block->dataDesc.captureId == _captureId)
            {
                reqNum++;
            }

            m_myImpl->finalizeIo(ioContext);
        }
    }

    return reqNum;
}

bool CBlockCapture::isCaptureStarted(int _captureId)
{
    return m_myImpl->m_captureState == CAPTURE_STARTED;
}

int CBlockCapture::releaseBlock(DataBlock_t* _block)
{
    return 0;
}

int CBlockCapture::testCapture(int _captureId, int _portIndex, packet_header_t* _pktHeader, byte* _pktData, bool _rmon)
{
    PacketMeta_t metaInfo = {0};
    metaInfo.basicAttr.wireLen = _pktHeader->len;
    metaInfo.basicAttr.pktLen = _pktHeader->caplen;

    if (g_env->m_config.justTestCapture == 1)
    {
    }
    else if (g_env->m_config.justTestCapture == 2)
    {
        ProcessPacketBasic(_pktData, &metaInfo);
    }
    else if (g_env->m_config.justTestCapture == 3)
    {
        ProcessPacketBasic(_pktData, &metaInfo);
        calcRmon(0, _pktHeader, &metaInfo, _pktData);
    }
    else if (g_env->m_config.justTestCapture == 4)
    {
        DataBlock_t* &dataBlock = m_bufBlock[_captureId];

        if (!dataBlock)
            dataBlock = m_myImpl->requestBlock(_captureId);

        ProcessPacketBasic(_pktData, &metaInfo);
        calcRmon(0, _pktHeader, &metaInfo, _pktData);

        int pktLen = sizeof(packet_header_t) + _pktHeader->caplen;
        int remain = DATA_BLOCK_SIZE - dataBlock->dataDesc.usedSize;
        if (remain >= pktLen)
        {
            byte* dst = dataBlock->data + dataBlock->dataDesc.usedSize;
            memcpy(dst, _pktHeader, sizeof(packet_header_t));
            dst += sizeof(packet_header_t);
            memcpy(dst, _pktData, _pktHeader->caplen);

            dataBlock->dataDesc.usedSize += pktLen;
        }
        else
        {
            DataBlock_t* block = m_myImpl->requestBlock(_captureId);
            m_myImpl->releaseBlock(dataBlock);
            dataBlock = block;
        }
    }
    else
    {
        handlePacket(_captureId, _portIndex, _pktHeader, _pktData, _rmon);
    }

    ++m_pktRecvCount[_captureId];
    m_byteRecvCount[_captureId] += _pktHeader->caplen;

    return 0;
}
