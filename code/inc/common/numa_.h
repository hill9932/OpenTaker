#ifndef __HILUO_NUMA_INCLUDE_H__
#define __HILUO_NUMA_INCLUDE_h__

#include "common.h"

namespace LiangZhu
{
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
}
