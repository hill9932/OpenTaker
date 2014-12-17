/**
* @Function:
*  This class manages the tbb pipeline task, pack index engine as task
* @Memo:
*  Created by hill, 6/17/2014
**/
#ifndef __HILUO_PIPELINE_TASK_INCLUDE_H__
#define __HILUO_PIPELINE_TASK_INCLUDE_H__

#include "common.h"
#include "app_base.h"
#include "lua_.h"
#include "packet_ring_class.h"
#include "tbb/pipeline.h"

#define MAX_TOKEN_COUNT 1024

class CAppTask : public tbb::filter
{
public:
    CAppTask(CAPPObj* _appObj, bool _isSerial);
    ~CAppTask();

    virtual void* operator()(void* item);

private:
    CAPPObj*   m_appObj;
};

class CBasicProcessTask : public CAppTask, public ISingleton<CBasicProcessTask>
{
public:
	CBasicProcessTask(bool _isSerial = false) : CAppTask(NULL, _isSerial) {};
	~CBasicProcessTask() {};

    virtual void* operator()(void* item);
};


/**
* This task get the packets from PacketRing and
* transfer them to decode engine
**/
class CCollectPacketTask : public CAppTask, public ISingleton<CCollectPacketTask>
{
public:
    CCollectPacketTask();

    int regWithAgent(const char* _ringName);
	void startRing() { m_packetRing.Start(); }
	int move2NextBlk() { return m_packetRing.Move2NextMetaBlk(); }

    virtual void* operator() (void* _item);

private:
    ModuleInfo_t* m_modInfo;
	CPacketRing m_packetRing;
	int         m_modOffset;
    u_int16     m_logInterval;
};


/**
 * @Function: this task recycle the packet to PacketRing
 **/
class CReturnPacketTask : public CAppTask, public ISingleton<CReturnPacketTask>
{
public:
    CReturnPacketTask() : CAppTask(NULL, true)
    {}

    virtual void* operator() (void* _item);
	void setColletTask(CCollectPacketTask* pCollectTask) { m_pCollectTask = pCollectTask; }

private:
	CCollectPacketTask* m_pCollectTask;
};

#endif
