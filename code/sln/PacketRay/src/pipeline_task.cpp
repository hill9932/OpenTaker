#include "pipeline_task.h"
#include "pipeline_manager.h"
#include "object_buffer.hxx"
#include "system_.h"

extern AppConf g_conf;

CAppTask::CAppTask(CAPPObj* _appObj, bool _isSerial)
: filter(_isSerial)
{
    m_appObj = _appObj;
}

CAppTask::~CAppTask()
{
}

void* CAppTask::operator()(void* _item)
{
	BlockToken* curToken = (BlockToken*)_item;

	if (curToken)
    {
        if (curToken->pMetaBlk)
        {
            m_appObj->processBlock(curToken);
        }
	}

    return _item;
}

void* CBasicProcessTask::operator()(void* _item)
{
	BlockToken* curToken = (BlockToken*)_item;

	if (curToken && curToken->pMetaBlk)
	{
		const u_int32 curPackets = curToken->pMetaBlk->size;
		for (u_int32 i = 0; i < curPackets; ++i)
		{
            ProcessPacketBasic(curToken->packetTokens[i].data, curToken->packetTokens[i].metaInfo);
		}
	}

	return _item;
}


/**
* This task get the packets from index engine and
* transfer them to decode engine
**/
CCollectPacketTask::CCollectPacketTask()
: CAppTask(NULL, true)
{
    m_modInfo = NULL;
    regWithAgent("OpenTaker");
}

int CCollectPacketTask::regWithAgent(const char* _ringName)
{
    const int DEFAULT_LOG_WAIT_TIME = 2;
    m_logInterval = g_conf.engine.log_interval;
 
    int index = 0;

    try 
    {
        std::string ringName;
        if (_ringName != NULL)
            ringName = _ringName;

        m_packetRing.Init(ringName, "PacketRay", 0);
    } 
    catch (...) 
    {
        RM_LOG_ERROR("Fail to init PacketRing.");
        return -1;
    }

    m_modInfo = (ModuleInfo_t*)m_packetRing.GetModuleInfo();
    if (!m_modInfo)
    {
        RM_LOG_ERROR("Fail to get module information.");
        return -1;
    }

    RM_LOG_INFO("Success to register the module '" <<   \
                m_modInfo->moduleName << \
                "' (" << m_packetRing.GetModuleID() << ")" << endl);
    return 0;
}


LiangZhu::CObjectPool<BlockToken> g_tokenPool(MAX_TOKEN_COUNT);

/**
* @Function: get packets from PacketRing
**/
void* CCollectPacketTask::operator()(void* item)
{
    int idleTime = 0;
    static u_int32 ringPos = 0;
    static bool isSeenStop = false;

    while (!isSeenStop)
	{
		PktMetaBlk_t* pMetaBlk = m_packetRing.GetNextFullMetaBlk();
		if (pMetaBlk)
		{
            assert((ringPos++ % m_packetRing.GetMetaBlockCount()) == pMetaBlk->blkPos);
			BlockToken* curToken = g_tokenPool.getObject();
			assert(curToken);

			curToken->pMetaBlk = pMetaBlk;
			curToken->flag = 0;
			const u_int32 curPackets = pMetaBlk->size;
			if (curToken->packetTokens.size() < curPackets)
			{
				curToken->packetTokens.resize(curPackets);
			}
			PacketMeta_t* firstPacket = m_packetRing.GetPktMetaInBlk(pMetaBlk, 0);
 			for (int i = 0; i < curPackets; ++i)
			{
				PacketMeta_t* packet = m_packetRing.GetPktMetaInBlk(pMetaBlk, i);
				assert(packet);
				curToken->packetTokens[i].metaInfo = packet;
				curToken->packetTokens[i].data = m_packetRing.GetPacketData(pMetaBlk->type, packet);
				curToken->packetTokens[i].flag = 0;
			}
            // is stop signal
            if (curPackets == 1 && firstPacket->basicAttr.pktLen == 0 && firstPacket->basicAttr.ts == 0)
            {
                isSeenStop = true;
            }
			return curToken;
		}
		else
		{
			if (++idleTime % 10 == 0)  SleepMS(MyMin(idleTime / 10, 1000));
			else LiangZhu::YieldCurThread();
		}
	}

    ringPos = 0;
    isSeenStop = false;

    RM_LOG_INFO("TBB prepare to exist.");

    return NULL;
}


void* CReturnPacketTask::operator() (void* _item)
{
	BlockToken* token = (BlockToken*)_item;
    if (token)
    {
		int ret = -1;
		if (token->pMetaBlk && (ret = m_pCollectTask->move2NextBlk()) < 0)
			RM_LOG_ERROR("Fail to move to next blk " << ret);

        g_tokenPool.release(token);
    }
    return NULL;
}

