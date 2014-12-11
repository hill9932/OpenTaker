/**
 * @Function:
 *  This is the index engine SDK
 * @Memo:
 *  Create by hill, 5/6/2014
 **/

#ifndef __HILUO_PACKET_RING_INCLUDE_H__
#define __HILUO_PACKET_RING_INCLUDE_H__

#include "common.h"
#include "structure.h"
#include "decode_basic.h"

#include <list>



#ifdef VPAGENT_EXPORTS

#ifdef WIN32
#define VPAGENT_API __declspec(dllexport)

#elif defined(LINUX)
#define VPAGENT_API
#endif

#else

#ifdef WIN32
#define VPAGENT_API __declspec(dllimport)

#elif defined(LINUX)
#define VPAGENT_API
#endif
#endif


typedef u_int32     ModuleID;
typedef void *      PktRingHandle_t;

#define CACHE_LINE_SIZE     64
#define MAX_MODULE_COUNT    20      // the maximum number of module supported now
#define MAX_MODULE_NAME     20      // the name size of module name shouldn't exceed
#define MAX_COLUMN_COUNT    20      // the maximum number of columns one module can register
#define MAX_COLUMN_NAME     21      // the name size of column name that module shouldn't exceed


#pragma pack(push, 8)

struct PktMetaBlk_t
{
    DataBlock_t *dataBlk;
    bool        isFinal;        // if this is the last meta block used by 'dataBlk'
    PacketType_e  type;           // packet type, DPDK / NAPATECH / ACCOLADE

    u_int32     blkPos;
    u_int32     offset;
    u_int32     size;
};

/**
 * @Function: keep the module register information
 **/
struct ModuleInfo_t
{
    byte*   blockViewStart;                 // the start address of mapped packet block memory
    byte*   metaViewStart;                  // the start address of mapped meta memory
    byte*   metaBlkViewStart;               // the start address of mapped meta block memory
    int     offset;                         // the offset of registered index
    volatile u_int32 blkPtr;                // point to the valid packet
    volatile u_int32 packetPtr;             // point to the valid packet
    char    moduleName[MAX_MODULE_NAME];    // the registered module name
};

struct ModuleIndex_t
{
    char    moduleName[MAX_MODULE_NAME];    // the registered module name
    char    colCount;
    char    colName[MAX_COLUMN_COUNT][MAX_COLUMN_NAME];
    char    colType[MAX_COLUMN_COUNT][MAX_COLUMN_NAME];
    int     colSize[MAX_COLUMN_COUNT];
};

#pragma pack(pop)


#ifdef __cplusplus
extern   "C"{
#endif

VPAGENT_API
void SetRingLogger(log4cplus::Logger* _logger);

/**
* @Function: Every process should call this function first to create the
*            necessary resource such as share memory.
* @Param _globalName:  the global resource name this process wants to join.
*                      since there may be multiple process running, so you have to
*                      specify this name.
* @Param _moduleName:  the module name of this process, a process always need one module.
* @Param _blockSize:   the size of share memory to keep data, alway the first process need to provide this.
* @Param _handler:     handler of the packet ring , which has the name '_globalName'
* @Return:
*  -1: error
*  module id
**/
VPAGENT_API
ModuleID InitPacketRing(PktRingHandle_t *_handler,
                        const tchar* _globalName,
                        const tchar* _moduleName,
                        u_int64 _blockSize,
                        u_int32 _metaBlkCapacity = 10000);

VPAGENT_API
ModuleID GetMyID(const PktRingHandle_t _handler, const char* _name);

/**
* @Function: every module should call this function to register itself before call other functions
* @Param _name: the module name, should be less than 10 chars
* @Param _colNum:  specify the size of following arrays
* @Param _colName: specify the name of every field.
* @Param _colType: specify the type of every field.
* @Param _colSize: specify the value size of every field.
* @Param _handler: specify the packet ring this module register to.
* @Param _offset[out]: the offset of ext
* @Return
*  -1: call InitPacketRing() first
*  -2: column type invalid, only support "int", "integer", "text", "numeric"
* @Warning:
*  If the module crash and restart, make sure the register values are not change,
*  since you will get the original offset which may corrupt the memory.
**/
VPAGENT_API
ModuleID RegMySelf(const char* _name,
                   const int   _colNum,
                   const char* _colName[],
                   const char* _colType[],
                   const int   _colSize[],
                   PktRingHandle_t _handler,
                   int*        _offset);

/**
* @Function: every module will call it to get the next available packet to handle
* @Param _handler: handler of the packet ring this module register to.
* @Param _id: the module id returned by RegMySelf.
* @Return:
*  PacketMeta_t*: the space to keep all the packet inforamtion
*  NULL
**/
VPAGENT_API
struct PacketMeta_t* GetNextPacket(PktRingHandle_t _handler, ModuleID _id);

VPAGENT_API
struct PktMetaBlk_t* GetNextMetaBlk(PktRingHandle_t _handler, ModuleID _id);

VPAGENT_API
struct PacketMeta_t* GetMetaInBlk(PktRingHandle_t _handler, 
                                  ModuleID _id,
                                  const PktMetaBlk_t *_metaBlk, 
                                  u_int32 _idx);

/**
* @Function: Tell the agent module have processed the packet and move forward the pointer.
* @Param _handler: handler of the packet ring that this module register to.
* @Param _id: the module id returned by RegMySelf
* @Return:
*  The current packet pointer
*/
VPAGENT_API
int MoveToNextPacket(PktRingHandle_t _handler, ModuleID _id, int _count);

VPAGENT_API
int Move2NextMetaBlk(PktRingHandle_t _handler, ModuleID _id, int _count);

VPAGENT_API
struct packet_header_t* GetPacketHeader(PacketType_e type,
                                        struct PacketMeta_t* _metaInfo,
                                        PktRingHandle_t _handler,
                                        ModuleID _modID,
                                        packet_header_t* _pktHeader);
VPAGENT_API
byte* GetPacketData(PacketType_e type, 
                    struct PacketMeta_t* _metaInfo,
                    PktRingHandle_t _handler, 
                    ModuleID _modID);

VPAGENT_API
ModuleInfo_t* GetModuleInfo(PktRingHandle_t _handler, ModuleID _id);

VPAGENT_API
bool IsPacketRingStop(const PktRingHandle_t _handler);

#ifdef __cplusplus
}
#endif



VPAGENT_API
ModuleID RegMySelf(const char* _name,
                   vector<string>& _colName,
                   vector<string>& _colType,
                   vector<int>& _colSize,
                   PktRingHandle_t _handler,
                   int*        _offset);

VPAGENT_API
void StopPacketRing(PktRingHandle_t _handler);

/************************************************************************
 *
 * These functions should not be called by 3rd module.
 *
 ************************************************************************/
VPAGENT_API
bool ReleasePacketRing(PktRingHandle_t _handler, ModuleID _moduleId, const char* _globalName);

/**
 * @Function: Called by VPEyes to start work.
 * @Memo: after it is called, engine will reject following register.
 **/
VPAGENT_API
int StartWork(PktRingHandle_t _handler);

/**
 * @Function: Get all the index registered by 3rd modules
 * @Return: the size of _vec
 **/
VPAGENT_API
int GetModuleIndex(PktRingHandle_t _handler, vector<ModuleIndex_t>& _vec);

/**
 * @Function: Get all the registered module information
 * @Return: the size of _vec
 **/
VPAGENT_API
int GetAllModuleInfo(PktRingHandle_t _handler, vector<int>& _vec);

/**
 * @Function: After the manager call the StartWork(), it will be become ready.
 **/
VPAGENT_API
bool IsAgentReady(PktRingHandle_t _handler);

VPAGENT_API
bool IsAgentEmpty(PktRingHandle_t _handler);

VPAGENT_API
int GetModuleCount(PktRingHandle_t _handler);

/**
 * @Function: get the PacketMeta according to the ring position
 * @Memo: IO finished routine will call this to get the PacketMeta and do some
 *        operation like add a record
 **/
VPAGENT_API
PacketMeta_t* GetOnePacket(PktRingHandle_t _handler, ModuleID _id, int _index);

VPAGENT_API
PktMetaBlk_t* GetOneMetaBlk(PktRingHandle_t _handler, ModuleID _id, int _index);

/**
 * @Function: The provider will call this to provide data
 * @Memo: Only this function will update the _G->g_tailPacket
 *  Multiple thread can call this
 **/
VPAGENT_API
PacketMeta_t* GetNextMetaSpace(PktRingHandle_t _handler,
                               u_int32* _pos, bool _wait = true);  // only should be called by module 0

// Only called by module 0
VPAGENT_API
PktMetaBlk_t* GetEmptyMetaBlk(PktRingHandle_t _handler, u_int32* _pos, bool _wait = true);

VPAGENT_API
bool IsMetaBlkFull(PktRingHandle_t _handler, PktMetaBlk_t* _metaBlk);

VPAGENT_API
u_int32 GetMetaBlkCapacity(PktRingHandle_t _handler, PktMetaBlk_t* _metaBlk);

VPAGENT_API
int GetNextPacket(PktRingHandle_t _handler,
                  ModuleID _id, int& _index, vector<PacketMeta_t*>& _packets);

VPAGENT_API
int GetNextMetaBlks(PktRingHandle_t _handler, ModuleID _id,
                    int& _index, list<PktMetaBlk_t*>& _metaBlks);

VPAGENT_API
u_int32 GetMetaCount(PktRingHandle_t _handler);

VPAGENT_API
u_int32 GetMetaBlkCount(PktRingHandle_t _handler);


#endif
