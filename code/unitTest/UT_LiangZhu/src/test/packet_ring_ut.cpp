#include "gtest/gtest.h"
#include "packet_ring_class.h"
#include "thread.h"

class CPacketRingTest : public ::testing::Test
{
public:
    CPacketRingTest()
    {
        m_conf.enable   = true;
        m_conf.ready    = false;
        m_makePacketThread = m_handlePacketThread = NULL;
    }

protected:
    virtual void SetUp()
    {
        m_makePacketThread   = new LiangZhu::CSimpleThread(MakePacketFunc, &m_conf);
        m_handlePacketThread = new LiangZhu::CSimpleThread(HandlePacketFunc, &m_conf);
        SetLogger(g_logger);
    }

    virtual void TearDown()
    {
        SAFE_DELETE(m_makePacketThread);
        SAFE_DELETE(m_handlePacketThread);
    }

protected:
    struct TestConf_t
    {
        bool	enable;
        bool	ready;
    };

    static int MakePacketFunc(void* _context);
    static int HandlePacketFunc(void* _context);

protected:
    TestConf_t	m_conf;
    LiangZhu::CSimpleThread*	m_makePacketThread;
    LiangZhu::CSimpleThread*	m_handlePacketThread;
};

int CPacketRingTest::MakePacketFunc(void* _context)
{
    if (!_context)  return -1;

    CProducerRingPtr packetRing = CProducerRingPtr(new CProducerRing());
    packetRing->Clear();
    packetRing->Init("CPacketRingTest", "CPacketRingTest", 10 * ONE_MB);

    ModuleInfo_t* modInfo = (ModuleInfo_t*)packetRing->GetModuleInfo();
    if (!modInfo)	return -1;

    TestConf_t* conf = (TestConf_t*)_context;
    while (!conf->ready)
        SleepSec(1);

    struct timeval tv;
    u_int64 ts = 0;
    gettimeofday(&tv, NULL);
    ts = (u_int64)tv.tv_sec * NS_PER_SECOND + tv.tv_usec * 1000;

    while (conf->enable)
    {

    }

    return 0;
}

int CPacketRingTest::HandlePacketFunc(void* _context)
{
    if (!_context)  return -1;
    TestConf_t* conf = (TestConf_t*)_context;

    while (!conf->ready)		SleepSec(1);
    while (conf->enable)
    {

    }

    return 0;
}


TEST_F(CPacketRingTest, WorkRound)
{
    m_conf.ready = true;
    m_conf.enable = false;
    m_makePacketThread->join();
    m_handlePacketThread->join();
}
