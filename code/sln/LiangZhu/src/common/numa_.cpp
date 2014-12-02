#include "numa_.h"

#ifdef WIN32
CNumaHome::CNumaHome()
{
    m_numaCount = 0;
}

int CNumaHome::getNumaNodeCount()
{
    if (m_numaCount == 0)
    {
        ULONG numaCount = 0;
        GetNumaHighestNodeNumber(&numaCount);
        m_numaCount = numaCount;
    }

    return m_numaCount;
}

int CNumaHome::getNumaCpuCount()
{
    return 0;
}

bool CNumaHome::isNumaEnable()
{
    ULONG numaCount = 0;
    return (bool)GetNumaHighestNodeNumber(&numaCount);
}

int CNumaHome::getCurrentNuma()
{
    UCHAR node;
    GetNumaProcessorNode(0, &node);
    return node;
}

#else

CNumaHome::CNumaHome()
{
    m_isNumaReady = (-1 != numa_available());
    m_numaCount = 0;
}

int CNumaHome::getNumaNodeCount()
{
    if (m_numaCount > 0)    return m_numaCount;

    if (numa_available() < 0)
        return 0;
    //m_numaCount = numa_num_possible_nodes();
    //m_numaCount = numa_max_possible_node();
    m_numaCount = numa_num_configured_nodes();
    return m_numaCount;
}

int CNumaHome::getNumaCpuCount()
{
    return numa_num_configured_cpus();
}

bool CNumaHome::isNumaEnable()
{
    return m_isNumaReady && getNumaNodeCount() > 0;
}

int CNumaHome::getCurrentNuma()
{
    int cpu = sched_getcpu();
    int node = numa_node_of_cpu(cpu);
    return node;
}

int CNumaHome::bindToNuma(int _numa)
{
    CStdString strNodeId;
    strNodeId.Format("%d", _numa);

    struct bitmask *nodemask =  numa_parse_nodestring(strNodeId.c_str());
    numa_bind(nodemask);
    return 0;
}

int bind_cpu(int cpu)
{
    int z;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    z = sched_setaffinity(getpid(), sizeof(mask), &mask);
    return z;
}

#endif

