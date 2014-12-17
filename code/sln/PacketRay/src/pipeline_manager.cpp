#include "pipeline_manager.h"
#include "pipeline_task.h"

CPipelineMgr::CPipelineMgr()
{
    m_pipeline  = NULL;
    m_isRunning = false;
}

CPipelineMgr::~CPipelineMgr()
{
    SAFE_DELETE(m_pipeline);
}

void CPipelineMgr::start()
{
    RM_LOG_INFO("PacketRay's pipeline start");
    m_isRunning = true;
    m_pipeline->run(MAX_TOKEN_COUNT);
    m_isRunning = false;
    clear();
    RM_LOG_INFO("PacketRay's pipeline stop");
}

void CPipelineMgr::init()
{
    SAFE_DELETE(m_pipeline);
    m_pipeline = new tbb::pipeline();
}

void CPipelineMgr::addTask(CAppTask* _task)
{
    assert(!m_isRunning);
    if (_task)
        m_pipeline->add_filter(*_task);
}

void CPipelineMgr::clear()
{
    assert(!m_isRunning);
    m_pipeline->clear();
}
