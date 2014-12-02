#ifndef __VP_NUMA_INCLUDE_H__
#define __VP_NUMA_INCLUDE_h__

#include "common.h"

#ifdef LINUX
#include <numa.h>
#endif

class CNumaHome : public ISingleton<CNumaHome>
{
public:
    CNumaHome();

    bool isNumaEnable();
    int  getNumaNodeCount();
    int  getNumaCpuCount();
    int  getCurrentNuma();
    int  bindToNuma(int _numa);

private:
    bool m_isNumaReady;
    int  m_numaCount;
};

#endif
