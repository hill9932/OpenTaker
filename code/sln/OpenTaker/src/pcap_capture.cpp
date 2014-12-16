#include "pcap_capture.h"
#include "system_.h"
#include "global.h"
#include "string_.h"

#define CAPTURE_TIME_OUT            500     // ms
#define PCAP_OPENFLAG_PROMISCUOUS   1

CPcapCapture::CPcapCapture()
{
    m_devHandle = NULL;
}

CPcapCapture::~CPcapCapture()
{
}

int CPcapCapture::scanLocalNICs()
{
    int curSize = m_localNICs.size();
    int pcapDevSize = 0;

    // check whether has detected
    for (u_int32 i = 0; i < m_localNICs.size(); ++i)
    {
        if (m_localNICs[i].type == DEVICE_TYPE_LIBPCAP)
            ++pcapDevSize;
    }

    if (pcapDevSize != 0)   return pcapDevSize;
    int portIndex = 0;
    if (curSize > 0)
        portIndex = m_localNICs[curSize - 1].portIndex + m_localNICs[curSize - 1].portCount;

#ifdef WIN32
    GetSysNICs2(m_localNICs);
    for (u_int32 i = curSize; i < m_localNICs.size(); ++i)
    {
        m_localNICs[i].name = "\\Device\\NPF_" + m_localNICs[i].name;
        m_localNICs[i].portIndex = portIndex++;
        m_localNICs[i].type = DEVICE_TYPE_LIBPCAP;
    }

#else
    int myPortIndex = 0;
    int myDevIndex = 0;

    //
    // get the device name by "lspci"
    //
    CStdString lspci = "lspci";
    FILE *stream = popen(lspci, "r" );
    ON_ERROR_LOG_LAST_ERROR_AND_DO(stream, ==, NULL, return -1);

    char buf[4096] = {0};
    char* z = fgets(buf, 4096, stream);
    while(z)
    {
        vector<CStdString> infos;
        StrSplit_s(buf, ": ", infos);

        if (infos.size() >= 2 && infos[0].find("Ethernet controller") != string::npos)
        {
            DeviceST device;
            device.desc = CStdString(buf).Trim(); //infos[1].Trim();

            CStdString slot = infos[0].substr(0, infos[0].find(" "));

            //
            // get detail information by "lspci -vmmks <address>"
            //
            lspci = "lspci -vmmks ";
            lspci += slot;

            //
            // get the interface name if has
            //
            CStdString busId = "0000:";
            busId += slot;
            lspci = "/sys/bus/pci/devices/";
            lspci += busId;
            lspci += "/net/";

            CStdString cmd = "ethtool ";
            vector<CStdString> subs;
            ListDir(lspci.c_str(), subs);
            if (subs.size() > 0)
            {
                device.desc += ", ";
                device.desc += subs[0];
                cmd += subs[0];
                device.name = subs[0];
            }

            device.linkSpeed = atoi(getField(cmd, "Speed:"));

            device.portStat[0].status = LINK_ON;
            device.myDevIndex = myDevIndex;
            device.myPortIndex = myPortIndex;
            device.portIndex = portIndex;
            device.type = DEVICE_TYPE_LIBPCAP;

            m_localNICs.push_back(device);
            portIndex += device.portCount;
            myPortIndex += device.portCount;
            myDevIndex++;
        }

        bzero(buf, 4096);
        z = fgets(buf, 4096, stream);
    }

    pclose(stream);
    
#endif

    return m_localNICs.size() - curSize;
}

bool CPcapCapture::openDevice_(int _index, const char* _devName)
{
    if (!_devName)	return false;

    char errBuf[PCAP_ERRBUF_SIZE] = { 0 };
    m_devHandle = pcap_open_live(_devName, 65535, PCAP_OPENFLAG_PROMISCUOUS, CAPTURE_TIME_OUT, errBuf);
    ON_ERROR_LOG_MESSAGE_AND_DO(m_devHandle, == , NULL, errBuf, return false);

 /*   pcap_t* pcapHandle = pcap_create(_devName, errBuf);
    ON_ERROR_LOG_MESSAGE_AND_DO(pcapHandle, == , NULL, errBuf, return false);
    
    int z = pcap_set_promisc(pcapHandle, PCAP_OPENFLAG_PROMISCUOUS);
    ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, pcap_geterr(pcapHandle), return false);

    z = pcap_set_snaplen(pcapHandle, 65535);
    ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, pcap_geterr(pcapHandle), return false);

    z = pcap_set_timeout(pcapHandle, CAPTURE_TIME_OUT);
    ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, pcap_geterr(pcapHandle), return false);

    z = -1;
    u_int32 buffSize = 200 * ONE_MB;
    while (z != 0 && buffSize > ONE_MB)
    {
        z = pcap_set_buffer_size(pcapHandle, buffSize);
        if (0 == z)
            z = pcap_activate(pcapHandle);

        buffSize /= 2;
    }
    RM_LOG_INFO("Success to set the pcap kernel buffer size to " << ChangeUnit(buffSize * 2, 1024, "B"));
    
    m_devHandle = pcapHandle;
    */
    return true;
}

bool CPcapCapture::closeDevice_()
{
    if (m_devHandle)
    {
        pcap_close(m_devHandle);
        m_devHandle = NULL;
    }

    return true;
}

int CPcapCapture::openFile(const char* _fileName)
{
    if (!_fileName) return -1;

    char* err = NULL;
    closeDevice();

    m_devHandle = pcap_open_offline(_fileName, err);
    ON_ERROR_LOG_MESSAGE_AND_DO(m_devHandle, == , NULL, err, return -1);

    return 0;
}

int CPcapCapture::startCapture_(void* _arg)
{
    CaptureContext_t* context = (CaptureContext_t*)_arg;

    struct  pcap_pkthdr *pktHeader = NULL;
    const   byte*   pktData = NULL;
    int     z = 0;
    u_int64 prevTimeStamp = 0;
    time_t  checkTime = 0;
    time(&checkTime);

    while (g_env->enable())
    {
        if (m_captureState[context->captureId] == CAPTURE_STOPPING)
        {
            CBlockCapture::handlePacket(context->captureId, m_device.portIndex, NULL, NULL, true);
            return 1;
        }

        if (!isCaptureStarted(context->captureId))
        {
            SleepSec(1);
            continue;
        }

        z = pcap_next_ex(m_devHandle, &pktHeader, &pktData);

        time_t now;
        time(&now);
        if (now - checkTime >= 5)
        {
            CBlockCapture::handlePacket(context->captureId, m_device.portIndex, NULL, NULL, true);    // flush the buffer
            checkTime = now;
        }

        if (z == 0)  // timeout
        {
            YieldCurThread();
            continue;
        }
        else if (-2 == z)   // last packet in the file
        {
            CBlockCapture::handlePacket(context->captureId, m_device.portIndex, NULL, NULL, true);    // flush the buffer
            break;  
        }
        else if (z < 0)   // error
        {
            RM_LOG_ERROR(pcap_geterr(m_devHandle));
            break;
        }

        packet_header_t header =
        {
            { pktHeader->ts.tv_sec, pktHeader->ts.tv_usec * 1000 },
            pktHeader->caplen,
            pktHeader->len
        };

        u_int64 ts = (u_int64)header.ts.tv_sec * NS_PER_SECOND + header.ts.tv_nsec;
        if (prevTimeStamp == 0)  prevTimeStamp = ts;
        if (ts <= prevTimeStamp)
        {
            ts = prevTimeStamp + 100;
            header.ts.tv_sec = ts / NS_PER_SECOND;
            header.ts.tv_nsec = ts % NS_PER_SECOND;
        }
        prevTimeStamp = ts;

        testCapture(context->captureId, m_device.portIndex, &header, (byte*)pktData, true);
    }

    RM_LOG_INFO("CPcapCapture Capturing exited.");

    return z;
}

int CPcapCapture::getStatistic(Statistic_t& _pcapStat)
{
    if (!isOpen())  return -1;

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

int CPcapCapture::getStatistic(Statistic_t& _pcapStat, int _port)
{
    if (!isOpen())  return -1;

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
    _pcapStat.rmonStat.pkts_512_1023    = m_portStats[i].rmonStat.pkts_512_1023;
    _pcapStat.rmonStat.pkts_1024_1518   = m_portStats[i].rmonStat.pkts_1024_1518;
    _pcapStat.rmonStat.pkts_1519_2047   = m_portStats[i].rmonStat.pkts_1519_2047;
    _pcapStat.rmonStat.pkts_2048_4095   = m_portStats[i].rmonStat.pkts_2048_4095;
    _pcapStat.rmonStat.pkts_4096_8191   = m_portStats[i].rmonStat.pkts_4096_8191;
    _pcapStat.rmonStat.pkts_8192_9018   = m_portStats[i].rmonStat.pkts_8192_9018;
    _pcapStat.rmonStat.pkts_9019_9198   = m_portStats[i].rmonStat.pkts_9019_9198;
    _pcapStat.rmonStat.pkts_oversize    = m_portStats[i].rmonStat.pkts_oversize;

    pcap_stat pcapStat = { 0 };
    int z = pcap_stats(m_devHandle, &pcapStat);
    _pcapStat.pktSeenByNIC = pcapStat.ps_recv;
    _pcapStat.bytesSeenByNIC = m_byteRecvCount[i] + 20 * _pcapStat.pktSeenByNIC;  // add the 20 B frame gap size;
    _pcapStat.pktDropCount = MyMax(pcapStat.ps_drop, pcapStat.ps_ifdrop);
    
    _pcapStat.index = m_devIndex;
    _pcapStat.port = _port;

    return 0;
}

int CPcapCapture::setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter)
{
    if (!isOpen() || !_rawFilter || !*_rawFilter) return -1;

    struct bpf_program fcode;

    // compile the filter
    u_int32 netmask = -1;

    int z = pcap_compile(m_devHandle, &fcode, _rawFilter, 1, netmask);
    ON_ERROR_LOG_MESSAGE_AND_DO(z, <, 0, pcap_geterr(m_devHandle), return z);

    // set the filter
    z = pcap_setfilter(m_devHandle, &fcode);
    ON_ERROR_LOG_MESSAGE_AND_DO(z, <, 0, pcap_geterr(m_devHandle), return z);

    pcap_freecode(&fcode);
    return z;
}

#ifdef LINUX
CStdString CPcapCapture::getField(CStdString& _file, const tchar* _key)
{
    if (_file.empty() || !_key) return "";

    FILE* stream = popen(_file.c_str(), "r");
    if (!stream)    return "";

    CStdString value;
    char buf[4096] = { 0 };
    char* z = fgets(buf, 4096, stream);
    while (z)
    {
        string ss = buf;
        size_t pos;
        if ((pos = ss.find(_key)) != string::npos)
        {
            char* tmp = strdup_t(ss.substr(pos + strlen_t(_key)).c_str());
            value = StrTrim(tmp);
            free(tmp);
            break;
        }

        bzero(buf, 4096);
        z = fgets(buf, 4096, stream);
    }

    pclose(stream);
    return value;
}
#endif
