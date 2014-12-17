/**
 * @Function:
 *  This class manages the tbb pipeline, load index engine as task
 * @Memo:
 *  Created by hill, 6/17/2014
 **/

#ifndef __HILUO_PIPELINE_MANAGE_INCLUDE_H__
#define __HILUO_PIPELINE_MANAGE_INCLUDE_H__

#include "common.h"
#include "util.hxx"
#include "pipeline_task.h"
#include "tbb/task_scheduler_init.h"

using namespace tbb;

class CPipelineMgr : public ISingleton<CPipelineMgr>
{
public:
    CPipelineMgr();
    ~CPipelineMgr();

    void start();
    void stop();
    void addTask(CAppTask*);
    void clear();

    void init();

private:
    task_scheduler_init m_tbbInit;
    tbb::pipeline       *m_pipeline;
    volatile bool       m_isRunning;
};


#endif
