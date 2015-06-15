#include "packet_ring.h"
#include "decode_basic.h"
#include "system_.h"
#include "string_.h"
#include "file_system.h"
#include "math_.h"

#include <boost/interprocess/detail/atomic.hpp>
#include <fstream>

#ifdef ENABLE_DPDK
#include <rte_mbuf.h>
#include <rte_eal.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_errno.h>
#endif

using namespace LiangZhu;

#define SHARE_MEM_GLOBAL_NAME   "VP_Global_SM"          // the share memory to keep the _G
RM_LOG_DEFINE("PacketRing");

#pragma pack(push, 8)
struct Global_t
{
    u_int64         g_blockSize;        // the size of share memory to store packets data
    u_int64         g_metaPoolSize;
    u_int32         g_metaCount;        // total number of meta item
    u_int32         g_metaSize;         // the meta size for every packet
    u_int32         g_metaBlkCapacity;  // the capacity of a single meta block

    int	            g_index;            // how many module registered
    int             g_offset;           // offset of ext meta

    u_int32         g_metaBlkCount;     // total count of meta block

    volatile u_int32 g_tailMetaBlk;
    volatile u_int32 g_tailPacket;
    bool             g_stopped;
    CRITICAL_SECTION g_mutex;

    ModuleInfo_t    g_moduleInfo[MAX_MODULE_COUNT];
    ModuleIndex_t   g_moduleIndex[MAX_MODULE_COUNT];
};
#pragma pack(pop)

byte* CreateBlockMemory(const tchar* _name, u_int64 _size);
byte* CreateHugeMemory(const tchar* _hugetlbfs, int _pageCount, int _numaId);

bool  CheckModValid(const Global_t *_G, ModuleID _id);
bool  CheckColumnType(const char* _colType);
int   FindModule(const Global_t *_G, const char* _name);

extern "C"
{
    void SetLogger(log4cplus::Logger* _logger)
    {
        g_logger = _logger;
    }

    ModuleID InitRing(PktRingHandle_t *_handler,
                       const tchar* _globalName,
                       const tchar* _moduleName,
                       u_int64 _blockSize,
                       u_int32 _metaBlkCapacity,
                       bool reset)
    {
        if (!_globalName)   _globalName = SHARE_MEM_GLOBAL_NAME;
        InitLog(GetAppDir() + "config/log4cplus.properties", _moduleName);

        byte* buf = (byte*)CreateBlockMemory(_globalName, sizeof(Global_t));
        if (!buf)   return -1;

        Global_t* _G = (Global_t*)buf;
        assert(_handler);
        *_handler = (PktRingHandle_t)_G;
        if (reset) ResetTheRing(*_handler);

        if (_G->g_index == 0 && _blockSize != 0)//_G->g_blockSize == 0)   // only the first process need to do this
        {
            bzero(buf, sizeof(Global_t));
            _G->g_blockSize = _blockSize;
            const int AVERAGE_PACKET_SIZE = 100;
            int size2 = LiangZhu::Log2(_blockSize / AVERAGE_PACKET_SIZE);

            u_int64 metaCount = (u_int64)(2 << size2); // MyMin(MAX_META_COUNT, (u_int64)(2 << size2));
            _G->g_metaPoolSize = metaCount * sizeof(PacketMeta_t);
            _G->g_stopped = false;

            _G->g_metaBlkCapacity = DEF_METABLOCK_CAPACITY;
            if (_metaBlkCapacity != 0) {
                _G->g_metaBlkCapacity = _metaBlkCapacity;
            }
            _G->g_metaBlkCount = (metaCount-1) / _G->g_metaBlkCapacity + 1;

            LiangZhu::InitCriticalSec(_G->g_mutex);
        }

        if (!_moduleName)   return 0;

        bool created = false;
        int index = FindModule(_G, _moduleName);
        if (-1 == index)
        {
            index = _G->g_index;
            created = true;
        }

        //
        // map to the share memory and keep the mapped address
        //
        int nameSize = MyMin((size_t)MAX_MODULE_NAME - 1, (size_t)strlen(_moduleName));
        strncpy(_G->g_moduleInfo[index].moduleName, _moduleName, nameSize);

        // create the block share memory
        CStdString blockShareName = _globalName;
        blockShareName += "_BlockSM";
        byte* addr = CreateBlockMemory(blockShareName, _G->g_blockSize + PAGE_SIZE);
        if (!addr)  return -1;
        //addr = (byte*)(((u_int64)addr + PAGE_SIZE - 1) & ~(u_int64)(PAGE_SIZE - 1));
        _G->g_moduleInfo[index].blockViewStart = addr;
        if (index == 0) bzero(addr, _G->g_blockSize);       // make memory hot

        // create the meta share memory
        blockShareName += "_MetaSM";
        addr = CreateBlockMemory(blockShareName, _G->g_metaPoolSize);
        if (!addr)  return -1;
        if (index == 0) bzero(addr, _G->g_metaPoolSize);   // make memory hot
        _G->g_moduleInfo[index].metaViewStart = addr;
        _G->g_moduleInfo[index].packetPtr = 0;

        // create meta block share memory,
        CStdString metaBlkShareName = _globalName;
        metaBlkShareName += "_MetaBlockSM";
        u_int32 metaBlockSize = _G->g_metaBlkCount * sizeof(PktMetaBlk_t);
        addr = CreateBlockMemory(metaBlkShareName, metaBlockSize);
        if (!addr) return -1;
        if (index == 0) bzero(addr, metaBlockSize);   // make memory hot
        _G->g_moduleInfo[index].metaBlkViewStart = addr;
        _G->g_moduleInfo[index].blkPtr = 0;

        strncpy(_G->g_moduleIndex[index].moduleName, _moduleName, nameSize);

        if (created) ++_G->g_index;
        return index;
    }

    ModuleID RegMySelf(const char* _name,
                       const int   _colNum,
                       const char* _colName[],
                       const char* _colType[],
                       const int   _colSize[],
                       PktRingHandle_t _handler,
                       int*        _offset)
    {
        if (IsRingReady(_handler) && _colNum > 0)  return -1;  // StartWork() has been called

        Global_t *_G = (Global_t *)_handler;

        SCOPE_LOCK_(_G->g_mutex);
        int index = FindModule(_G, _name);
        if (index == -1)    return -1;

        if (_colNum > MAX_COLUMN_COUNT ||
            (_colNum > 0 && (!_colName || !_colType || !_colSize)))
            return -2;

        // has registered
        if (_G->g_moduleInfo[index].offset)
        {
            if (_offset)    *_offset = _G->g_moduleInfo[index].offset;
            return index;
        }

        //
        // keep the column information
        //
        _G->g_moduleInfo[index].offset = _G->g_offset;
        for (int i = 0; i < _colNum; ++i)
        {
            if (!CheckColumnType(_colType[i]))  return -2;

            strncpy(_G->g_moduleIndex[index].colName[i],
                    _colName[i],
                    MyMin((size_t)MAX_COLUMN_NAME - 1, (size_t)strlen(_colName[i])));
            strncpy(_G->g_moduleIndex[index].colType[i],
                    _colType[i],
                    MyMin((size_t)MAX_COLUMN_NAME - 1, (size_t)strlen(_colType[i])));
            _G->g_moduleIndex[index].colSize[i] = _colSize[i];
            _G->g_offset += _colSize[i];
        }
        _G->g_moduleIndex[index].colCount = _colNum;

        return index;
    }

    ModuleID GetMyID(const PktRingHandle_t _handler, const char* _name)
    {
        return FindModule((const Global_t *)_handler, _name);
    }

    ModuleInfo_t* GetModuleInfo(PktRingHandle_t _handler, ModuleID _id)
    {
        Global_t *_G = (Global_t *)_handler;
        if (!CheckModValid(_G, _id)) return NULL;
        return &_G->g_moduleInfo[_id];
    }

    PacketMeta_t* GetNextPacket(PktRingHandle_t _handler, ModuleID _id)
    {
        Global_t *_G = (Global_t *)_handler;
        assert(_G);

        ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
        if (!modInfo->blockViewStart || !modInfo->metaViewStart)
            return NULL;

        int nRetry = 0;
        int curPtr = modInfo->packetPtr;

        // mod 0 is also the s2d, it will depend on the last mod
        ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
        while (++nRetry < 100)
        {
            int limPtr = (_id == 1 || _id == depModId)
                                ? (_G->g_tailPacket & _G->g_metaCount)
                                : _G->g_moduleInfo[depModId].packetPtr;
            if (curPtr != limPtr)
            {
                PacketMeta_t* packet =
                    (PacketMeta_t*)(modInfo->metaViewStart + _G->g_metaSize * curPtr);
                //assert(packet->ringPos == curPtr);

                if (packet->basicAttr.pktLen == 0)     // packet is not ready
                    return NULL;

                return packet;
            }

            YieldCurThread();
        }

        return NULL;
    }

    int MoveToNextPacket(PktRingHandle_t _handler, ModuleID _id, int _count)
    {
        Global_t *_G = (Global_t *)_handler;
        assert(_G);

        ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
        if (!modInfo->blockViewStart ||
            !modInfo->metaViewStart)    return -2;

        volatile u_int32& curPtr = modInfo->packetPtr;

        // mod 0 is also the s2d, it will depend on the last mod
        ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
        u_int32 limPtr;
        if (_id == 1 || _id == depModId)
            limPtr = (_G->g_tailPacket & _G->g_metaCount);
        else
            limPtr = _G->g_moduleInfo[depModId].packetPtr;

        if (limPtr < curPtr)
            limPtr += GetMetaCount(_handler);   // limPtr has a round trip

        if (curPtr + _count <= limPtr)
        {
            curPtr += _count;
            curPtr &= _G->g_metaCount;
            return curPtr;
        }

        return -3;
    }

    PktMetaBlk_t* GetNextMetaBlk(PktRingHandle_t _handler, ModuleID _id)
    {
        Global_t *_G = (Global_t *)_handler;
        assert(_G);

        ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
        if (!modInfo->blockViewStart || !modInfo->metaViewStart)
            return NULL;

        int nRetry = 0;
        int curPtr = modInfo->blkPtr;

        // mod 0 is also the s2d, it will depend on the last mod
        ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
        while (++nRetry < 100)
        {
            int limPtr = (_id == 1 || _id == depModId)
                                ? (_G->g_tailMetaBlk % _G->g_metaBlkCount)
                                : _G->g_moduleInfo[depModId].blkPtr;
            if (curPtr != limPtr)
            {
                PktMetaBlk_t *metaBlk =
                    (PktMetaBlk_t *)(modInfo->metaBlkViewStart + curPtr*sizeof(PktMetaBlk_t));

                if (metaBlk->size == 0)     // meta block is not ready
                    return NULL;

                return metaBlk;
            }

            YieldCurThread();
        }

        return NULL;
    }

    int Move2NextMetaBlk(PktRingHandle_t _handler, ModuleID _id, int _count)
    {
        Global_t *_G = (Global_t *)_handler;
        assert(_G);

        ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
        if (!modInfo->blockViewStart || !modInfo->metaViewStart)
            return -2;

        volatile u_int32& curPtr = modInfo->blkPtr;

        // mod 0 is also the s2d, it will depend on the last mod
        ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
        u_int32 limPtr;
        if (_id == 1 || _id == depModId)
            limPtr = _G->g_tailMetaBlk % _G->g_metaBlkCount;
        else
            limPtr = _G->g_moduleInfo[depModId].blkPtr;

        if (limPtr < curPtr)
            limPtr += GetMetaBlkCount(_handler);   // limPtr has a round trip

        if (curPtr + _count <= limPtr)
        {
            int tmpCur = (curPtr + _count) % _G->g_metaBlkCount;
            curPtr = tmpCur;
            return curPtr;
        }

        return -3;
    }

    PacketMeta_t* GetMetaInBlk(PktRingHandle_t _handler, ModuleID _id,
                               const PktMetaBlk_t *_metaBlk, u_int32 _idx)
    {
        if (_metaBlk == NULL) return NULL;
        if (_metaBlk->size > 0 && _idx >= _metaBlk->size) return NULL;
        Global_t *_G = (Global_t *)_handler;
        assert(_G);

        if (_metaBlk->size == 0 && _idx >= _G->g_metaBlkCapacity) return NULL;

        ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
        if (!modInfo->blockViewStart || !modInfo->metaViewStart)
            return NULL;

        PacketMeta_t *metaInfo = (PacketMeta_t*)(modInfo->metaViewStart +
                                                 _G->g_metaSize * (_metaBlk->offset + _idx));

        return metaInfo;
    }

    packet_header_t* GetPacketHeader(PacketType_e type, PacketMeta_t* _metaInfo,
                                     PktRingHandle_t _handler, ModuleID _modID, packet_header_t* _pktHeader)
    {
        if (!_metaInfo || _metaInfo->headerAddr == (u_int64)-1) return NULL;
        packet_header_t* pktHeader = NULL;

        if (type == PACKET_NAPATECH)    // napatech packet
        {
            pktHeader = (packet_header_t*)_metaInfo->headerAddr;
        }
#ifdef ENABLE_DPDK
        else if (type == PACKET_DPDK)
        {
            // if packets are allocated by DPDK, they are saved one by one
            // rather than in block
            rte_mbuf* mbuf = (rte_mbuf*)_metaInfo->headerAddr;
            pktHeader = rte_pktmbuf_mtod(mbuf, packet_header_t*);
        }
#endif
        else
        {
            ModuleInfo_t* modInfo = GetModuleInfo(_handler, _modID);
            if (!modInfo)   return NULL;

            pktHeader = (packet_header_t*)(modInfo->blockViewStart + _metaInfo->headerAddr);
        }

#ifdef LINUX
        assert(_metaInfo->indexValue.ts == (u_int64)pktHeader->ts.tv_sec * NS_PER_SECOND + pktHeader->ts.tv_nsec);
#endif

        return pktHeader;
    }

    byte* GetPacketData(PacketType_e type, PacketMeta_t* _metaInfo,
                        PktRingHandle_t _handler, ModuleID _modID)
    {
        if (!_metaInfo || _metaInfo->headerAddr == (u_int64)-1) return NULL;
        byte* data = NULL;

        if (type == PACKET_NAPATECH)    // napatech packet
        {
            byte* pktHeader = (byte*)_metaInfo->headerAddr;
            data = pktHeader + sizeof(packet_header_t);
        }
#ifdef ENABLE_DPDK
        else if (type == PACKET_DPDK)
        {
            // if packets are allocated by DPDK, they are saved one by one
            // rather than in block
            rte_mbuf* mbuf = (rte_mbuf*)_metaInfo->headerAddr;
            byte* pktHeader = rte_pktmbuf_mtod(mbuf, byte*);
            data = pktHeader + sizeof(packet_header_t);
        }
#endif
        else
        {
            ModuleInfo_t* modInfo = GetModuleInfo(_handler, _modID);
            if (!modInfo)   return NULL;

            if (_metaInfo->headerAddr == (u_int64)-1)
                return NULL;

            byte* pktHeader = (byte*)(modInfo->blockViewStart + _metaInfo->headerAddr);
            data = pktHeader + sizeof(packet_header_t);
        }

        return data;
    }

#ifdef ENABLE_DPDK
    bool InitDPDK(const char* _cmdLine)
    {
        int z = 0;
        int argc = 20;
        char* argv[20] = { 0 };
        char* buf = Str2Argv(_cmdLine, &argv[0], argc);

        z = rte_eal_init(argc, argv);

        free(buf);
        return z >= 0;
    }
#endif

    bool IsRingReady(PktRingHandle_t _handler)
    {
        Global_t *_G = (Global_t *)_handler;
        if (!_G)    return false;
        return _G->g_metaCount != 0;
    }

    bool IsRingEmpty(PktRingHandle_t _handler)
    {
        Global_t *_G = (Global_t *)_handler;
        if (!_G)    return true;
        ModuleInfo_t*  modInfo = (ModuleInfo_t*)GetModuleInfo(_handler, 0);
        if (!modInfo)   return true;

        return (_G->g_tailMetaBlk % _G->g_metaBlkCount) == modInfo->blkPtr;
    }

    bool IsRingStop(PktRingHandle_t _handler)
    {
        if (!_handler) return true;
        return ((const Global_t *)_handler)->g_stopped;
    }

    bool InitPackage(const char* _cmdLine)
    {
        return true;
    }
}

/**
 * @Function: after calling this, reject later register
 **/
int StartWork(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);

    if (_G->g_blockSize <= 0 || _G->g_index <= 0)   return -1;
    _G->g_metaSize = sizeof(PacketMeta_t)+_G->g_offset;
    assert(_G->g_offset == 0);   // don't support extension currently

    _G->g_metaCount = (_G->g_metaPoolSize / _G->g_metaSize) - 1;
    assert((_G->g_metaCount + 1) % (2 ^ 10) == 0);  // should be 2 ^ xx

    return 0;
}

int GetModuleCount(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    if (!_G)    return 0;
    return _G->g_index;
}

u_int32 GetMetaCount(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    return _G->g_metaCount + 1;
}

u_int32 GetMetaBlkCount(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    return _G->g_metaBlkCount;
}

void StopRing(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    if (_G) _G->g_stopped = true;
}

void ResetTheRing(PktRingHandle_t _handler)
{
    Global_t *_G = (Global_t *)_handler;
    if (_G) bzero(_G, sizeof(Global_t));
}

ModuleID RegMySelf( const char* _name,
                    vector<string>& _colName,
                    vector<string>& _colType,
                    vector<int>& _colSize,
                    PktRingHandle_t _handler,
                    int*        _offset)
{
    if (_colName.size() != _colType.size() || _colName.size() != _colSize.size())
        return -1;

    int colNum = _colName.size();
    const char** colName = new const char*[colNum];
    const char** colType = new const char*[colNum];
    int* colSize = new int[colNum];

    for (int i = 0; i < colNum; ++i)
    {
        colName[i] = _colName[i].c_str();
        colType[i] = _colType[i].c_str();
        colSize[i] = _colSize[i];
    }

    ModuleID ID =
        RegMySelf(_name, colNum, colName, colType, colSize, _handler, _offset);
    delete[] colName;
    delete[] colType;
    delete[] colSize;

    return ID;
}

int GetModuleIndex(PktRingHandle_t _handler, vector<ModuleIndex_t>& _vec)

{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    for (int i = 0; i < _G->g_index; ++i)
    {
        _vec.push_back(_G->g_moduleIndex[i]);
    }

    return _vec.size();
}

int GetAllModuleInfo(PktRingHandle_t _handler, vector<int>& _vec)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    _vec.push_back(_G->g_tailPacket & _G->g_metaCount);

    for (int i = 1; i < _G->g_index; ++i)
    {
        _vec.push_back(_G->g_moduleInfo[i].packetPtr);
    }

    _vec.push_back(_G->g_moduleInfo[0].packetPtr);
    return _vec.size();
}

/**
* @Function: find module register information according to module name
**/
int FindModule(const Global_t *_G, const char* _name)
{
    assert(_G);
    if (!_name || !_G)  return -1;
    int nameSize = MyMin((size_t)MAX_MODULE_NAME - 1, (size_t)strlen(_name));

    for (int i = 0; i < MAX_MODULE_COUNT; ++i)
    {
        if (!_G->g_moduleInfo[i].blockViewStart)
            return  -1;
        if (strncmp(_G->g_moduleInfo[i].moduleName, _name, nameSize) == 0)
            return i;
    }

    return -1;
}


bool CheckModValid(const Global_t *_G, ModuleID _id)
{
    assert(_G);
    if (_id >= MAX_MODULE_COUNT) return false;
    const ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
    return modInfo->blockViewStart != NULL && modInfo->metaViewStart != NULL;
}

bool CheckColumnType(const char* _colType)
{
    if (!_colType)  return false;
    if (0 == stricmp(_colType, "int"))
        return true;
    else if (0 == stricmp(_colType, "integer"))
        return true;
    else if (0 == stricmp(_colType, "text"))
        return true;
    else if (0 == stricmp(_colType, "numeric"))
        return true;

    return false;
}

PacketMeta_t* GetOnePacket(PktRingHandle_t _handler, ModuleID _id, int _index)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
    if (!modInfo->blockViewStart ||
        !modInfo->metaViewStart)    return NULL;

    _index &= _G->g_metaCount;

    PacketMeta_t* packet =
        (PacketMeta_t*)(modInfo->metaViewStart + _G->g_metaSize * _index);
    //assert(packet->indexValue.pktLen);
    return packet;
}

PktMetaBlk_t* GetOneMetaBlk(PktRingHandle_t _handler, ModuleID _id, int _index)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
    if (!modInfo->blockViewStart || !modInfo->metaViewStart)
        return NULL;

    _index %= _G->g_metaBlkCount;
    PktMetaBlk_t* metaBlk =
        (PktMetaBlk_t*)(modInfo->metaBlkViewStart + sizeof(PktMetaBlk_t)*_index);
    return metaBlk;
}

PacketMeta_t* GetNextMetaSpace(PktRingHandle_t _handler, u_int32* _pos, bool _wait)
{
    ModuleInfo_t* modInfo = (ModuleInfo_t*)GetModuleInfo(_handler, 0);

    Global_t *_G = (Global_t *)_handler;
    assert(_G);
retry:
    u_int32 tailPos = _G->g_tailPacket & _G->g_metaCount;
    u_int32 nextPos = (tailPos + 1) & _G->g_metaCount;
    u_int32 limitPtr = modInfo->packetPtr;

    if (nextPos == limitPtr)  // round trip, may drop packets
    {
        if (!_wait) return NULL;
        YieldCurThread();
        goto retry;
    }

    tailPos = INTERLOCKED_INCREMENT(&_G->g_tailPacket) - 1;
    tailPos &= _G->g_metaCount;

    PacketMeta_t* metaInfo = (PacketMeta_t*)(modInfo->metaViewStart + _G->g_metaSize * tailPos);
    while (metaInfo->basicAttr.pktLen != 0)
    {
        YieldCurThread();
        if (!_wait) return NULL;
    }

    if (_pos)   *_pos = tailPos;
    return metaInfo;
}

#define SAFE_DISTANCE   2

PktMetaBlk_t* GetEmptyMetaBlk(PktRingHandle_t _handler, u_int32* _pos, bool _wait)
{
    ModuleInfo_t* modInfo = (ModuleInfo_t*)GetModuleInfo(_handler, 0);
    if (!modInfo || !modInfo->blockViewStart || !modInfo->metaViewStart)
        return NULL;

    Global_t *_G = (Global_t *)_handler;
    assert(_G);

retry:
    u_int32 tailPos  = _G->g_tailMetaBlk % _G->g_metaBlkCount;
    u_int32 nextPos  = (tailPos + 1) % _G->g_metaBlkCount;
    u_int32 checkPos = (tailPos + SAFE_DISTANCE) % _G->g_metaBlkCount;
    u_int32 limitPtr = modInfo->blkPtr;

    if (checkPos == limitPtr)  // round trip
    {
        if (!_wait) return NULL;
        YieldCurThread();
        goto retry;
    }

    PktMetaBlk_t *metaBlk =
        (PktMetaBlk_t*)(modInfo->metaBlkViewStart + sizeof(PktMetaBlk_t) * tailPos);
    if (metaBlk->size != 0)
    {
        if (!_wait) return NULL;
        YieldCurThread();
        goto retry;
    }

    //INTERLOCKED_INCREMENT(&_G->g_tailMetaBlk);
    if (tailPos != boost::interprocess::ipcdetail::atomic_cas32(&_G->g_tailMetaBlk, nextPos, tailPos))   // move to the next
        goto retry;

    assert(metaBlk->blkPos == 0 || metaBlk->blkPos == tailPos);
    metaBlk->blkPos = tailPos;

    assert(metaBlk->offset == 0 || 
           metaBlk->offset == tailPos * _G->g_metaBlkCapacity);

    metaBlk->offset = tailPos * _G->g_metaBlkCapacity;
    if (_pos)   *_pos = tailPos;
    return metaBlk;
}

bool IsMetaBlkFull(PktRingHandle_t _handler, PktMetaBlk_t* _metaBlk)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);

    return _metaBlk->size >= GetMetaBlkCapacity(_handler, _metaBlk);
}

u_int32 GetMetaBlkCapacity(PktRingHandle_t _handler, PktMetaBlk_t* _metaBlk)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);

    if (_metaBlk->offset + _G->g_metaBlkCapacity > _G->g_metaCount)
        return _G->g_metaCount - _metaBlk->offset;

    return _G->g_metaBlkCapacity;
}

int GetNextPacket(PktRingHandle_t _handler,
                  ModuleID _id, int& _index, vector<PacketMeta_t*>& _packets)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
    if (!modInfo->blockViewStart || !modInfo->metaViewStart)
        return 0;

    int& curPtr = _index;
    // XXX mod 0 is also the s2d, it will depend on the last mod
    ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
    int limPtr;
    if (_id == 1 || _id == depModId)
        limPtr = (_G->g_tailPacket & _G->g_metaCount);
    else
        limPtr = _G->g_moduleInfo[depModId].packetPtr;

    u_int32 distance;
    if (curPtr > limPtr)
        distance = (limPtr + GetMetaCount(_handler) - curPtr);
    else
        distance = limPtr - curPtr;

    while (distance > 0)
    {
        PacketMeta_t* packet =
            (PacketMeta_t*)(modInfo->metaViewStart + _G->g_metaSize * curPtr);
        if (packet->basicAttr.pktLen == 0)     // this packet is not ready
            break;

        _packets.push_back(packet);
        ++curPtr &= _G->g_metaCount;
        --distance;
    }

    return _packets.size();
}

int GetNextMetaBlks(PktRingHandle_t _handler, ModuleID _id,
                    int& _index, list<PktMetaBlk_t*>& _metaBlks)
{
    Global_t *_G = (Global_t *)_handler;
    assert(_G);
    ModuleInfo_t* modInfo = &_G->g_moduleInfo[_id];
    if (!modInfo || !modInfo->blockViewStart || !modInfo->metaViewStart)
        return 0;

    int& curPtr = _index;
    // XXX mod 0 is also the s2d, it will depend on the last mod
    ModuleID depModId = _id == 0 ? _G->g_index - 1 : _id - 1;
    int limPtr;
    if (_id == 1 || _id == depModId)
        limPtr = _G->g_tailMetaBlk % _G->g_metaBlkCount;
    else
        limPtr = _G->g_moduleInfo[depModId].blkPtr;

    u_int32 distance;
    if (curPtr > limPtr)
        distance = (limPtr + GetMetaBlkCount(_handler) - curPtr);
    else
        distance = limPtr - curPtr;

    while (distance > 0)
    {
        PktMetaBlk_t *metaBlk =
            (PktMetaBlk_t*)(modInfo->metaBlkViewStart + sizeof(PktMetaBlk_t) * curPtr);
        if (metaBlk->size == 0)     // this meta block is not ready
            break;

        _metaBlks.push_back(metaBlk);
        int tmpPtr = (curPtr + 1) % _G->g_metaBlkCount;
        curPtr = tmpPtr;
        --distance;
    }

    return _metaBlks.size();
}


#ifdef WIN32
#include "main_win32.hxx"
#else
#include "main_linux.hxx"
#endif

