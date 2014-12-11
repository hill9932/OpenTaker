/**
* @Function
*  This file defines the class which use WinPcap/Libpcap to do capture job
* @Memo
*  Created by hill, 4/27/2014
**/

#ifndef __HILUO_PCAP_CAPTURE_INCLUDE_H__
#define __HILUO_PCAP_CAPTURE_INCLUDE_H__

#include "block_capture.h"

class CPcapCapture : public CBlockCapture
{
public:
    CPcapCapture();
    virtual ~CPcapCapture();

    static  int  scanLocalNICs();

    virtual bool isOpen() { return m_devHandle != NULL; }
    virtual int  getStatistic(Statistic_t& _pcapStat);
    virtual int  getStatistic(Statistic_t& _pcapStat, int _port);

    int openFile(const char* _fileName);

protected:
    virtual bool openDevice_(int _index, const char* _devName);
    virtual bool closeDevice_();

    virtual int  startCapture_(void* _arg);
    virtual int  setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter);

#ifdef LINUX
    static CStdString getField(CStdString& _file, const tchar* _key);
#endif

private:
    pcap_t* m_devHandle;

};

#endif // __VP_PCAP_CAPTURE_INCLUDE_H__
