#include "packet_ring_class.h"

CPacketRing::CPacketRing()
: m_ringHandler(NULL), m_ringName("")
, m_moduleId((ModuleID)-1), m_metaBlkPickPos(0)
{
}

CPacketRing::~CPacketRing()
{
}

void CPacketRing::Init(const std::string& ringName,
                       const std::string& moduleName,
                       u_int64 pktBlkSize,
                       u_int32 metaBlkCapacity/* = DEF_METABLOCK_CAPACITY*/)
{
    m_metaBlkPickPos = 0;

    const char *name = NULL;
    if (!ringName.empty()) {
        name = ringName.c_str();
        m_ringName = ringName;
    }

    m_moduleId = InitAgent(&m_ringHandler, name,
                           moduleName.c_str(), pktBlkSize, metaBlkCapacity);
    if ((int)m_moduleId < 0) {
        //LOG_ERROR("Failed to init packet ring for module "
        //             << moduleName << ", block memory size: " << pktBlkSize);
        THROW_EXCEPTION(InitPacketRing, "Get an invalid module ID");
    }
}

ModuleInfo_t* CPacketRing::GetModuleInfo()
{
    return ::GetModuleInfo(m_ringHandler, m_moduleId);
}

ModuleID CPacketRing::RegisterModMetaIdx(const std::string& moduleName,
                                         const std::vector<string>& colNames,
                                         const std::vector<string>& colTypes,
                                         const std::vector<u_int32>& colSizes,
                                         u_int32* metaOffset)
{
    /**
     * XXX Should module id be checked ?
     */
    return RegMySelf(moduleName.c_str(),
                     (std::vector<string>&)colNames,
                     (std::vector<string>&)colTypes,
                     (std::vector<int>&)colSizes,
                     m_ringHandler,
                     (int*)metaOffset);
}

u_int32 CPacketRing::GetModuleIndex(std::vector<ModuleIndex_t>& indices)
{
    return ::GetModuleIndex(m_ringHandler, indices);
}

u_int32 CPacketRing::GetAllModulePosInfo(std::vector<u_int32>& ringPosList)
{
    return GetAllModuleInfo(m_ringHandler, (std::vector<int> &)ringPosList);
}

u_int32 CPacketRing::GetModuleCount()
{
    return ::GetModuleCount(m_ringHandler);
}

PktMetaBlk_t* CPacketRing::GetNextFullMetaBlk()
{
    if (m_metaBlks.size() == 0) {
        int sz = GetNextMetaBlks(m_ringHandler, m_moduleId,
                                 (int&)m_metaBlkPickPos, m_metaBlks);
        if (sz == 0) {
            return NULL;
        }
    }
    PktMetaBlk_t *metaBlk = m_metaBlks.front();
    if (metaBlk) {
        m_metaBlks.pop_front();
    }

    return metaBlk;
}

PacketMeta_t* CPacketRing::GetPktMetaInBlk(const PktMetaBlk_t *metaBlk, u_int32 index)
{
    return GetMetaInBlk(m_ringHandler, m_moduleId, metaBlk, index);
}

u_int32 CPacketRing::Move2NextMetaBlk(u_int32 blkCount/* = 1*/)
{
    return ::Move2NextMetaBlk(m_ringHandler, m_moduleId, blkCount);
}

PktMetaBlk_t* CPacketRing::GetMetaBlkByIdx(u_int32 index)
{
    return GetOneMetaBlk(m_ringHandler, m_moduleId, index);
}

packet_header_t* CPacketRing::GetPacketHeader(PacketType_e type, PacketMeta_t *metaInfo, packet_header_t* pktHeader)
{
    return ::GetPacketHeader(type, metaInfo, m_ringHandler, m_moduleId, pktHeader);
}

byte* CPacketRing::GetPacketData(PacketType_e type, PacketMeta_t* metaInfo)
{
    return ::GetPacketData(type, metaInfo, m_ringHandler, m_moduleId);
}

bool CPacketRing::IsStopped()
{
    return IsAgentStop(m_ringHandler);
}

bool CPacketRing::IsReady()
{
    return IsAgentReady(m_ringHandler);
}

bool CPacketRing::IsEmpty()
{
    return IsAgentEmpty(m_ringHandler);
}

void CPacketRing::Start()
{
    StartWork(m_ringHandler);
}

void CPacketRing::Stop()
{
    StopAgent(m_ringHandler);
}

bool CPacketRing::Release()
{
    return ReleaseAgent(m_ringHandler, m_moduleId,
                        m_ringName.empty() ? NULL : m_ringName.c_str());
}

bool CPacketRing::Clear(const std::string& ringName)
{
    return ReleaseAgent(NULL, m_moduleId, ringName.empty() ? NULL : ringName.c_str());
}

u_int32 CPacketRing::GetMetaCount()
{
    return ::GetMetaCount(m_ringHandler);
}

u_int32 CPacketRing::GetMetaBlockCount()
{
    return ::GetMetaBlkCount(m_ringHandler);
}

bool CPacketRing::IsStopSignal(const PktMetaBlk_t *metaBlock)
{
    if (metaBlock && metaBlock->dataBlk == NULL && metaBlock->isFinal && metaBlock->size == 1) {
        PacketMeta_t *metaInfo = GetPktMetaInBlk(metaBlock, 0);
        assert(metaInfo);
        return (metaInfo->indexValue.ts == 0 && metaInfo->indexValue.pktLen == 0);
    }

    return false;
}

bool CPacketRing::IsTimeoutSignal(const PktMetaBlk_t *metaBlock)
{
    if (metaBlock && metaBlock->dataBlk == NULL && metaBlock->isFinal && metaBlock->size == 1) {
        PacketMeta_t *metaInfo = GetPktMetaInBlk(metaBlock, 0);
        assert(metaInfo);
        return (metaInfo->indexValue.ts != 0 && metaInfo->indexValue.pktLen == 0);
    }

    return false;
}

// ---------------------------------------------------------------
//  Here we go, new stuff, new world, welcome to object-oriented.
// ---------------------------------------------------------------
#ifndef MAX_CAPTURE_THREAD
#define MAX_CAPTURE_THREAD 4
#endif

#ifndef MAX_RINGS_PER_ANIC
#define MAX_RINGS_PER_ANIC 64
#endif


/**
 * packet block id, `m_pktBlkID`, is initialized to -1, for the sake of easier
 * code writing in WaitNextEmptyPktBlk()
 */
CProducerRing::CProducerRing()
: m_pktBlockPool(NULL), m_pktBlkID(-1), m_pktBlkPoolNum(0)
{
    /**
     * Use the bigger value of max capture thread and max ring number of
     * accolade NIC, as vector size.
     *
     * The purpose here is to unify accolade and other NICs. With accolade NIC,
     * the packet block is per ring, unlike regular NIC (per capture thread).
     * Which means, the meta block currently used also need to be per ring,
     * which, then, requires bigger vector space.
     *
     */
    m_curMetaBlock.resize(MyMax(MAX_CAPTURE_THREAD, MAX_RINGS_PER_ANIC));
    std::for_each(m_curMetaBlock.begin(), m_curMetaBlock.end(), zero<PktMetaBlk_t *>);

    m_usedMetaInBlk.resize(MyMax(MAX_CAPTURE_THREAD, MAX_RINGS_PER_ANIC));
    std::for_each(m_usedMetaInBlk.begin(), m_usedMetaInBlk.end(), zero<u_int32>);
}

CProducerRing::~CProducerRing()
{
    DeleteArray(m_pktBlockPool);
    Release();
}

void CProducerRing::PreparePktBlkPool(u_int32 maxPrdcrNum, u_int32 poolSize)
{
    if (m_pktBlockPool != NULL && m_pktBlkPoolNum == maxPrdcrNum)
        return ; // Job done

    assert(m_pktBlockPool == NULL);

    m_pktBlkPoolNum = maxPrdcrNum;
    m_pktBlockPool = new PacketBlockPool[m_pktBlkPoolNum];
    if (m_pktBlockPool == NULL) {
        RM_LOG_ERROR("OOM, failed to prepare " << maxPrdcrNum << " packet block pool(s)");
        THROW_EXCEPTION(OutOfMemory, "Prepare packet block pool failed");
    }

    /**
     * Initialize packet block pool in shared memory
     */
    ModuleInfo_t* modInfo = GetModuleInfo();
    u_int32 blockCount = poolSize / sizeof(DataBlock_t);
    for (u_int32 i = 0; i < m_pktBlkPoolNum; ++i) {
        byte* startAddr = modInfo->blockViewStart + i * poolSize;
        m_pktBlockPool[i].resetMem((DataBlock_t*)startAddr, blockCount);
    }
}

// Only shoule be called by producer module
PktMetaBlk_t* CProducerRing::GetNextEmptyMetaBlk(u_int32 *pos)
{
    return GetEmptyMetaBlk(m_ringHandler, pos, false);
}

PktMetaBlk_t* CProducerRing::WaitNextEmptyMetaBlk(u_int32 *pos)
{
    return GetEmptyMetaBlk(m_ringHandler, pos);
}

void CProducerRing::FreeFullMetaBlk(PktMetaBlk_t *metaBlk)
{
    assert(metaBlk != NULL);

    metaBlk->isFinal = false;
    metaBlk->dataBlk = NULL;
    metaBlk->size = 0; // this meta block is available again.

    /**
     * TODO Move packet block release action here, instead of managed by every
     * capture, for code readability.
     */
}

DataBlock_t* CProducerRing::GetNextEmptyPktBlk(u_int32 _captureId)
{
    DataBlock_t* block = m_pktBlockPool[_captureId].tryGetBlock();
    if (block != NULL) {
        bzero(&block->dataDesc, sizeof(DataBlockDesc_t));
        block->dataDesc.id = INTERLOCKED_INCREMENT(&m_pktBlkID);
        block->dataDesc.captureId = _captureId;
    }

    return block;
}

DataBlock_t* CProducerRing::WaitNextEmptyPktBlk(u_int32 _captureId)
{
    DataBlock_t* block = m_pktBlockPool[_captureId].getBlock();
    bzero(&block->dataDesc, sizeof(DataBlockDesc_t));
    block->dataDesc.id = INTERLOCKED_INCREMENT(&m_pktBlkID);
    block->dataDesc.captureId = _captureId;

    return block;
}

void CProducerRing::FreeFullPktBlk(u_int32 _captureId, DataBlock_t *pktBlk)
{
    assert(_captureId < m_pktBlkPoolNum);
    assert(pktBlk != NULL);
    m_pktBlockPool[_captureId].release(pktBlk);
}

bool CProducerRing::IsMetaBlkFull(u_int32 captureId)
{
    return m_usedMetaInBlk[captureId] >=
        GetMetaBlkCapacity(m_ringHandler, m_curMetaBlock[captureId]);
}

PacketMeta_t* CProducerRing::GetMeta4PktBlk(u_int32 captureId,
                                            DataBlock_t *dataBlk)
{
    if (m_curMetaBlock[captureId] == NULL || IsMetaBlkFull(captureId)) {
        // Give current meta block back to ring
        DoneMetaBlk(captureId);

        // Wait for a new empty one
        m_curMetaBlock[captureId] = WaitNextEmptyMetaBlk(NULL);
        assert(m_curMetaBlock[captureId]->size == 0);
        assert(!m_curMetaBlock[captureId]->isFinal);

        m_usedMetaInBlk[captureId] = 0;
        m_curMetaBlock[captureId]->dataBlk = dataBlk;
        if (dataBlk != NULL)
            m_curMetaBlock[captureId]->type = dataBlk->dataDesc.type;
    }

    assert(m_curMetaBlock[captureId]->dataBlk == dataBlk);
    return GetPktMetaInBlk(m_curMetaBlock[captureId],
                           m_usedMetaInBlk[captureId]++);
}

void CProducerRing::FinalMetaBlk(u_int32 captureId)
{
    if (!m_curMetaBlock[captureId])
        return;

    m_curMetaBlock[captureId]->isFinal = true;
    DoneMetaBlk(captureId);
}

u_int32 CProducerRing::GetCurMetaBlkPos(u_int32 captureId)
{
    return m_curMetaBlock[captureId]->blkPos;
}

void CProducerRing::DoneMetaBlk(u_int32 captureId)
{
    /**
     * Do nothing, if not a valid meta block, or never been used
     */
    if (m_curMetaBlock[captureId] == NULL || m_usedMetaInBlk[captureId] == 0)
        return;

    /**
     * Let other modules know this block is ready to proceed
     */
    m_curMetaBlock[captureId]->size = m_usedMetaInBlk[captureId];
    m_curMetaBlock[captureId] = NULL;
}

void CProducerRing::SendStopSignal()
{
    PktMetaBlk_t *stopSignal = WaitNextEmptyMetaBlk(NULL);
    assert(stopSignal->dataBlk == NULL);
    PacketMeta_t *meta = GetPktMetaInBlk(stopSignal, 0);
    meta->indexValue.ts = 0;
    meta->indexValue.pktLen = 0;
    stopSignal->isFinal = true;

    stopSignal->size = 1;
}

void CProducerRing::SendTimeoutSignal(u_int64 ts)
{
    assert(ts != 0);

    PktMetaBlk_t *timeoutSignal = WaitNextEmptyMetaBlk(NULL);
    assert(timeoutSignal->dataBlk == NULL);
    PacketMeta_t *meta = GetPktMetaInBlk(timeoutSignal, 0);
    meta->indexValue.ts = ts;
    meta->indexValue.pktLen = 0;
    timeoutSignal->isFinal = true;

    timeoutSignal->size = 1;
}
