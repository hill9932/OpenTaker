/**
 * @Function: This class mimic a NIC to capture data
 **/
#ifndef __HILUO_VIRTUAL_CAPTURE_INCLUDE_H__
#define __HILUO_VIRTUAL_CAPTURE_INCLUDE_H__

#include "block_capture.h"
#include "pcap_file.h"

class CVirtualCapture : public CBlockCapture
{
public:
    CVirtualCapture();
    virtual ~CVirtualCapture();
    static  int  scanLocalNICs();

    virtual bool isOpen() { return m_pcapFile.isValid(); }

    virtual int  getStatistic(Statistic_t& _pcapStat);
    virtual int  getStatistic(Statistic_t& _pcapStat, int _port);

protected:
    virtual bool openDevice_(int _index, const char* _devName);
    virtual bool closeDevice_();

    virtual int  startCapture_(void* _arg);
    virtual int  setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter);

    virtual int  prepareResource(const CaptureConfig_t& _config);
    virtual int  releaseResource();

private:
    int handlePacket(int _captureId);

private:
    CPCAPFile   m_pcapFile;
};

#endif