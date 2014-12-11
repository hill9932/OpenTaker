#ifndef __HILUO_PACKET_RING_CLASS_H__
#define __HILUO_PACKET_RING_CLASS_H__

#include "packet_ring.h"
#include "fixsize_buffer.h"
#include "boost/shared_ptr.hpp"
#include <list>

const u_int32 DEF_METABLOCK_CAPACITY = 10000;

/**
 * CPacketRing, interfaces for indexing modules
 *
 * The indexing modules care about meta blocks, not packet blocks (for now, may
 * change in future).
 *
 */
class VPAGENT_API CPacketRing
{
public:
    CPacketRing();
    virtual ~CPacketRing();
    virtual void Init(const std::string& ringName,
                      const std::string& moduleName,
                      u_int64 pktBlkSize,
                      u_int32 metaBlkCapacity = DEF_METABLOCK_CAPACITY);
    virtual ModuleID GetModuleID() { return m_moduleId; };
    virtual ModuleInfo_t* GetModuleInfo();

    virtual ModuleID RegisterModMetaIdx(const std::string& moduleName,
                                        const std::vector<string>& colNames,
                                        const std::vector<string>& colTypes,
                                        const std::vector<u_int32>& colSizes,
                                        u_int32* metaOffset);
    virtual u_int32 GetModuleIndex(std::vector<ModuleIndex_t>& indices);
    virtual u_int32 GetAllModulePosInfo(std::vector<u_int32>& ringPosList);
    virtual u_int32 GetModuleCount();

    virtual PktMetaBlk_t* GetNextFullMetaBlk();
    virtual PacketMeta_t* GetPktMetaInBlk(const PktMetaBlk_t *metaBlk, u_int32 index);
    virtual u_int32 Move2NextMetaBlk(u_int32 blkCount = 1);
    virtual PktMetaBlk_t* GetMetaBlkByIdx(u_int32 index);

public:
    virtual packet_header_t* GetPacketHeader(PacketType_e type, PacketMeta_t *metaInfo, packet_header_t* pktHeader);
    virtual byte* GetPacketData(PacketType_e type, PacketMeta_t* metaInfo);

    virtual bool IsStopped();
    virtual bool IsReady();
    virtual bool IsEmpty();
    virtual void Start();
    virtual void Stop();
    virtual bool Release();
    virtual bool Clear(const std::string& ringName = "");

    virtual u_int32 GetMetaCount();
    virtual u_int32 GetMetaBlockCount();

    virtual bool IsStopSignal(const PktMetaBlk_t *metaBlock);
    virtual bool IsTimeoutSignal(const PktMetaBlk_t *metaBlock);

protected:
    DEFINE_EXCEPTION(InitPacketRing);

    PktRingHandle_t m_ringHandler;
    std::string m_ringName;

    ModuleID m_moduleId;
    u_int32 m_metaBlkPickPos;
    std::list<PktMetaBlk_t*> m_metaBlks;
};

/**
 * CProducerRing, packet ring operating interfaces for capturing(or packet store
 * reader) and S2D.
 *
 * This is working procedure:
 *
 *  1) gets empty packet block here, (by capturing or packet store reader,
 *     WaitNextEmptyPktBlk())
 *  2) throws packets in, by copying or by assigning pointer, (by capturing or
 *     packet store reader)
 *  3) gets empty meta block, (by capturing or packet store reader,
 *     WaitNextEmptyMetaBlk(), FOR NOW)
 *  4) fills in every meta in meta block, (by capturing or packet store reader,
 *     GetMeta4PktBlk(), FOR NOW)
 *  5) let indexing modules know the meta block is ready to proceed, (by
 *     capturing or packet store reader, FinalMetaBlk(), FOR NOW)
 *  6) releases meta block, (by S2D, FreeFullMetaBlk())
 *  7) releases packet block. (by S2D, FreeFullPktBlk())
 *
 */
class VPAGENT_API CProducerRing : public CPacketRing
{
public:
    CProducerRing();
    virtual ~CProducerRing();

    virtual void PreparePktBlkPool(u_int32 maxPrdcrNum, u_int32 poolSize);

    virtual DataBlock_t* GetNextEmptyPktBlk(u_int32 _captureId);
    virtual DataBlock_t* WaitNextEmptyPktBlk(u_int32 _captureId);
    virtual void FreeFullPktBlk(u_int32 _captureId, DataBlock_t *pktBlk);

    virtual PktMetaBlk_t* GetNextEmptyMetaBlk(u_int32 *pos);
    virtual PktMetaBlk_t* WaitNextEmptyMetaBlk(u_int32 *pos);
    virtual void FreeFullMetaBlk(PktMetaBlk_t *metaBlk);

public:
    virtual PacketMeta_t* GetMeta4PktBlk(u_int32 captureId, DataBlock_t *dataBlk);
    virtual bool IsMetaBlkFull(u_int32 captureId);
    virtual void FinalMetaBlk(u_int32 captureId);
    virtual u_int32 GetCurMetaBlkPos(u_int32 captureId);

    virtual void SendStopSignal();
    virtual void SendTimeoutSignal(u_int64 ts);

protected:
    virtual void DoneMetaBlk(u_int32 captureId);

protected:
    typedef LiangZhu::CFixBufferPool<DataBlock_t> PacketBlockPool;

    std::vector<PktMetaBlk_t*> m_curMetaBlock;
    std::vector<u_int32> m_usedMetaInBlk;
    PacketBlockPool *m_pktBlockPool;
    atomic_t m_pktBlkID;
    u_int32 m_pktBlkPoolNum;
};

typedef boost::shared_ptr<CPacketRing> CPacketRingPtr;
typedef boost::shared_ptr<CProducerRing> CProducerRingPtr;

#endif // __PACKET_RING_H__
