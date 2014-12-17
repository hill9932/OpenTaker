/**
 * @Function:
 *  all the decode engine should implement this interface
 * @Memo:
 *  Created by hill, 6/18/2014
 **/

#ifndef __HILUO_APPBASE_INCLUDE_H__
#define __HILUO_APPBASE_INCLUDE_H__

#include "packet_ray.h"
#include "packet_ring.h"
#include "protocol_header_l4.h"
#include <list>
#include <vector>

#define FLOW_TIMEOUT_VALUE      120     // when there is no data with 2 min, just timeout the flow

#define APP_COMMAND_CHECK_STAT  0x1
#define APP_COMMAND_OUT_STAT    0x2
#define APP_COMMAND_FORCE_LOG   0x4



/**
 * @Function: the data item flows along the tbb pipeline.
 **/
struct PacketToken
{
    byte* data;                 // point to the packet data from the ethernet header
    PacketMeta_t* metaInfo;     // include the packet basic meta data include the 5 tuples
    int flag;                   // APP_COMMAND_CHECK_STAT

    u_int32  getAppType() { return metaInfo->basicAttr.appType;  }
    bool isTransport()
    {
        return metaInfo && metaInfo->basicAttr.isTcpUdp();
    }

    bool isTCP()
    {
        return metaInfo && metaInfo->basicAttr.isTcp();
    }

    bool isUDP()
    {
        return metaInfo && metaInfo->basicAttr.isUdp();
    }

    bool getIpv6Addr(net_in6_addr &_src, net_in6_addr &_dst)
    {
        if (metaInfo && metaInfo->basicAttr.isIpv6())
        {
            ipv6_header_t* curIpv6Header = (ipv6_header_t*)(data + metaInfo->layer3Offset);
            _src = curIpv6Header->srcAddr;
            _dst = curIpv6Header->dstAddr;
            return true;
        }
        return false;
    }
};

struct BlockToken
{
	PktMetaBlk_t*			pMetaBlk;
	u_int64					flag;
    u_int64                 checkTime;
	void*					blkModuleData;
	vector<PacketToken>		packetTokens;
};


/**
 * @Function: every decode engine should implement their own object
 **/
class CAPPObj
{
public:
    CAPPObj() { m_colPos = -1;  m_needProcessPacket = true; m_mustSerial = true; }
    virtual ~CAPPObj() {}

    virtual vector<CStdString>  getTableName() = 0;
    virtual vector<CStdString>  getCreateTableSql() = 0;
    virtual int  getBasicIndexNames(vector<string>& _colName, vector<string>& _colType, vector<int>& _colSize) { return 0; }

    void setBasicIndexPos(int _pos) { m_colPos = _pos; }
    bool isMustSerial() { return m_mustSerial; }

    bool checkPacket(const PacketToken* _packetToken)
    {
        if (!_packetToken || !_packetToken->metaInfo || !_packetToken->data ||
            _packetToken->metaInfo->basicAttr.ts == 0 ||
            _packetToken->metaInfo->basicAttr.pktLen == 0  ||
            _packetToken->metaInfo->basicAttr.srcIP == 0 ||
            _packetToken->metaInfo->basicAttr.dstIP == 0 ||
            _packetToken->metaInfo->basicAttr.srcIP == _packetToken->metaInfo->basicAttr.dstIP)
            return false;

        return true;
    }

    int processPacket(PacketToken* _packetToken)
    {
        if (!checkPacket(_packetToken)) return -1;
        if (_packetToken->flag & APP_COMMAND_CHECK_STAT)
        {
            outputCounter(_packetToken->metaInfo->basicAttr.ts / NS_PER_SECOND, false);  // check in sec unit
        }

        return doProcessPacket_(_packetToken);
    }

    void processBlock(BlockToken* _blockToken)
    {
        if (m_needProcessPacket)
        {
            doProcessBlockStart_(_blockToken);
            const u_int32 curPackets = _blockToken->pMetaBlk->size;
            for (u_int32 i = 0; i < curPackets; ++i)
            {
                processPacket(&_blockToken->packetTokens[i]);
            }
            doProcessBlockStop_(_blockToken);
        }
        else
        {
            doProcessBlock_(_blockToken);
        }
    }

    void forceLog(time_t _now)
    {
        doForceLog(_now);
    }

protected:
    virtual CStdString getTableFields() = 0;
    virtual int doProcessPacket_(PacketToken* _packetToken) = 0;
    virtual int outputCounter(time_t _now, bool bForceLog) = 0;
    virtual void doForceLog(time_t _now) = 0;
    virtual void doProcessBlock_(BlockToken* _blockToken){}
    virtual void doProcessBlockStart_(BlockToken* _blockToken){}
    virtual void doProcessBlockStop_(BlockToken* _blockToken){}

protected:
    bool m_needProcessPacket;
    bool m_mustSerial;
    int  m_colPos;
};

struct engine_t
{
    int                 top_count;              // specify at most count of flow to output
    int                 log_interval;
    vector<std::string> enable_modules;
};

struct FlowFileInfo
{
    CStdString  filePath;       // specify where to read the trace file
    CStdString  fileFormat;     // specify the fields the trace file include
    CStdString  tableName;      // specify the table name into which to insert
    int retry;                  // record the retry number to insert into database
};

/**
 * @Function: this variable will be shard by all the decode modules
 **/
struct AppConf
{
    CStdString  outputDir;                  // the path of target trace files
    int         threadNum;
    engine_t    engine;
    list<FlowFileInfo>  cnterFileList;      // every index module should add the trace file into this queue
    CRITICAL_SECTION    listMutex;          // protect the cnterFileList
    bool        stop;                       // a flag to tell the index modules to stop work
};


/**
 * @Function: define the base class of flow object which keep the flow value and it's hash key
 * @Memo: KEY is the value type, HASH is the hash type
 **/
template<class KEY, class HASH>
class FlowObj
{
public:
	FlowObj() {}
	FlowObj(const KEY& _key, const HASH& _hash) : m_key(_key), m_hash(_hash)
    {}

    virtual ~FlowObj() {};

    void init(const KEY& _key, const HASH& _hash) { m_key = _key; m_hash = _hash; reset(); }
    const HASH& getHash() const   { return m_hash;    }
    const KEY&  getKey()  const   { return m_key;     }
    u_int32     getType() const   { return m_type;    }
    void        setType(u_int32 _type) { m_type = _type; }
    int         getPortNum(){ return m_portNum; }
    void        setPortNum(int _portNum) { m_portNum = _portNum; }

    virtual u_int64  getFirstSeenTime() = 0;
    virtual u_int64  getLastSeenTime()  = 0;
    virtual u_int64  getActiveTime()    = 0;
    virtual void reset() {}
    virtual bool wouldTimeout() { return true; }
    virtual int  getTimeoutValue() { return FLOW_TIMEOUT_VALUE; }
    virtual bool isTimeOut(time_t _now) { return wouldTimeout() && (_now - getLastSeenTime() / NS_PER_SECOND > getTimeoutValue()); }

private:
    KEY     m_key;
    HASH    m_hash;
    u_int32 m_type;
    int     m_portNum;
};


/**
* @Function: the shared library export the following functions
**/
typedef std::vector<CAPPObj*> APPObjList;

PR_API
CAPPObj* CreateAppObject(AppConf& _appConf);

PR_API bool CreateAppObjList(AppConf& _appConf, APPObjList& _appObjList);

typedef CAPPObj* (*CreateAppObject_F)(AppConf& _appConf);
typedef bool(*CreateAppObjList_F)(AppConf& _appConf, APPObjList& _appObjList);


#endif  // END __VP_APPBASE_INCLUDE_H__
