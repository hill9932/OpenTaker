#include "virtual_capture.h"
#include "capture_store.h"
#include "file_system.h"
#include "global.h"
#include "time_.h"


CVirtualCapture::CVirtualCapture()
{
}

CVirtualCapture::~CVirtualCapture()
{
}

int CVirtualCapture::scanLocalNICs()
{
    int curSize = m_localNICs.size();
    int devSize = 0;

    // check whether has detected
    for (unsigned int i = 0; i < m_localNICs.size(); ++i)
    {
        if (m_localNICs[i].type == DEVICE_TYPE_VIRTUAL)
            ++devSize;
    }

    if (devSize != 0)   return devSize;
    int portIndex = 0;
    if (curSize > 0)
        portIndex = m_localNICs[curSize - 1].portIndex + m_localNICs[curSize - 1].portCount;

    DeviceST dev;
    dev.name = "Capture packets from pcap file";
    dev.desc = "Virtual Capture Device V1.0";

    dev.portStat[0].status = LINK_ON;
    dev.portCount = 1;
    dev.portIndex = portIndex;
    dev.myDevIndex = 0;
    dev.myPortIndex = 0;
    dev.linkSpeed = 10000;  // 10 Gbps
    dev.type = DEVICE_TYPE_VIRTUAL;

    m_localNICs.push_back(dev);

    return m_localNICs.size() - curSize;
}

int CVirtualCapture::getStatistic(Statistic_t& _pcapStat)
{
    for (unsigned int i = 0; i < m_device.portCount; ++i)
    {
        Statistic_t portStat = { 0 };
        if (0 == getStatistic(portStat, i))
        {
            _pcapStat.pktSeenByNIC += portStat.pktSeenByNIC;
            _pcapStat.bytesSeenByNIC += portStat.bytesSeenByNIC;
            _pcapStat.pktDropCount += portStat.pktDropCount;
            _pcapStat.pktErrCount += portStat.pktErrCount;
            _pcapStat.rmonStat.brdcst_pkts += portStat.rmonStat.brdcst_pkts;
            _pcapStat.rmonStat.mcst_pkts += portStat.rmonStat.mcst_pkts;
        }
    }

    _pcapStat.index = m_devIndex;
    _pcapStat.port = -1;
    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        _pcapStat.pktRecvCount += m_pktRecvCount[i];
        _pcapStat.bytesRecvCount += m_byteRecvCount[i];
    }

    return 0;
}

int CVirtualCapture::getStatistic(Statistic_t& _pcapStat, int _port)
{
    bzero(&_pcapStat, sizeof(_pcapStat));

    int i = _port;
    _pcapStat.rmonStat.total_pkts   = m_portStats[i].rmonStat.total_pkts;
    _pcapStat.rmonStat.total_bytes  = m_portStats[i].rmonStat.total_bytes;
    _pcapStat.rmonStat.err_pkts     = m_portStats[i].rmonStat.err_pkts;
    _pcapStat.rmonStat.drop_pkts    = m_portStats[i].rmonStat.drop_pkts;
    _pcapStat.rmonStat.brdcst_pkts  = m_portStats[i].rmonStat.brdcst_pkts;
    _pcapStat.rmonStat.mcst_pkts    = m_portStats[i].rmonStat.mcst_pkts;
    _pcapStat.rmonStat.pkts_lt_64   = m_portStats[i].rmonStat.pkts_lt_64;
    _pcapStat.rmonStat.pkts_eq_64   = m_portStats[i].rmonStat.pkts_eq_64;
    _pcapStat.rmonStat.pkts_65_127  = m_portStats[i].rmonStat.pkts_65_127;
    _pcapStat.rmonStat.pkts_128_255 = m_portStats[i].rmonStat.pkts_128_255;
    _pcapStat.rmonStat.pkts_256_511 = m_portStats[i].rmonStat.pkts_256_511;
    _pcapStat.rmonStat.pkts_512_1023  = m_portStats[i].rmonStat.pkts_512_1023;
    _pcapStat.rmonStat.pkts_1024_1518 = m_portStats[i].rmonStat.pkts_1024_1518;
    _pcapStat.rmonStat.pkts_1519_2047 = m_portStats[i].rmonStat.pkts_1519_2047;
    _pcapStat.rmonStat.pkts_2048_4095 = m_portStats[i].rmonStat.pkts_2048_4095;
    _pcapStat.rmonStat.pkts_4096_8191 = m_portStats[i].rmonStat.pkts_4096_8191;
    _pcapStat.rmonStat.pkts_8192_9018 = m_portStats[i].rmonStat.pkts_8192_9018;
    _pcapStat.rmonStat.pkts_9019_9198 = m_portStats[i].rmonStat.pkts_9019_9198;
    _pcapStat.rmonStat.pkts_oversize  = m_portStats[i].rmonStat.pkts_oversize;

    _pcapStat.pktSeenByNIC   = m_pktRecvCount[i];
    _pcapStat.bytesSeenByNIC = m_byteRecvCount[i];

    _pcapStat.index = m_devIndex;
    _pcapStat.port = _port;
    return 0;
}

bool CVirtualCapture::openDevice_(int _index, const char* _devName)
{
    CStdString filePath = g_env->m_config.engine.pcapFile;
    if (!IsFileExist(filePath) || 
      0 != m_pcapFile.open(filePath, ACCESS_READ, FILE_OPEN_EXISTING, false, false))
    {
        RM_LOG_ERROR("Fail to open: " << filePath);
        return false;
    }

    RM_LOG_INFO("Success to open: " << filePath);
    
    return true;
}

bool CVirtualCapture::closeDevice_()
{
    if (!isOpen())  return true;
    return 0 == m_pcapFile.close();
}

int CVirtualCapture::prepareResource(CaptureConfig_t& _config)
{
    return 0;
}

int CVirtualCapture::releaseResource()
{
    return 0;
}

int CVirtualCapture::startCapture_(void* _arg)
{
    CaptureContext_t* context = (CaptureContext_t*)_arg;
    RM_LOG_INFO("Start capturing port " << context->portId);

    struct timeval tsVal;
    u_int64 ts = 0;
    while (g_env->enable())
    {
        if (m_captureState[context->captureId] == CAPTURE_STOPPING)
        {
            CBlockCapture::handlePacket(context->captureId, m_device.portIndex, NULL, NULL, true);
            return 1;
        }

        // Wait until all threads are ready, current capture thread and store
        // thread, maybe process thread, too.
        if (!isCaptureStarted(context->captureId))
        {
            SleepSec(1);
            continue;
        }

        u_int64 fileSize = m_pcapFile.getFileSize();
        byte* fileData = m_pcapFile.getBuf();
        if (!fileData)  return -1;

        u_int64 offset = sizeof(pcap_file_header);
        int pktLen = 0;
        packet_header_t* pktHeader = NULL;
        byte *pktData = NULL;

        if (0 == ts)
        {
            gettimeofday(&tsVal, NULL);
            ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;
        }
        while (offset < fileSize)
        {
            gettimeofday(&tsVal, NULL);
            ts = (u_int64)tsVal.tv_sec * NS_PER_SECOND + tsVal.tv_usec * 1000;

            ts += 2;
            pktHeader = (packet_header_t*)(fileData + offset);
            pktLen = sizeof(packet_header_t) + pktHeader->caplen;
            pktData = (byte*)pktHeader + sizeof(packet_header_t);
            offset += pktLen;

//            assert(pktLen == 76 || pktLen == 488);

            packet_header_t newPktHeader = *pktHeader;
            newPktHeader.ts.tv_sec = ts / NS_PER_SECOND;
            newPktHeader.ts.tv_nsec = ts % NS_PER_SECOND;

            if (g_env->m_config.justTestCapture > 0)
                testCapture(context->captureId, m_device.portIndex, &newPktHeader, pktData, true);
            else
                CBlockCapture::handlePacket(context->captureId,
                                            m_device.portIndex,
                                            &newPktHeader, pktData, true);
            SleepUS(1);

            if (m_captureState[context->captureId] == CAPTURE_STOPPING)
                break;

            if (!g_env->enable())   return 1;
        }

        //break;
    }

    RM_LOG_INFO("CVirtualCapture Capturing exited.");

    return 1;
}

int CVirtualCapture::setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter)
{
    return -1;
}
