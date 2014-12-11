/**
 * @Function:
 *  This class keep the packets into a block first and process and save to disk using aio
 * @Memo:
 *  Created by hill, 4/30/2014
 **/
#ifndef __HILUO_BLOCK_CAPTURE_INCLUDE_H__
#define __HILUO_BLOCK_CAPTURE_INCLUDE_H__

#include "capture.h"
#include "fixsize_buffer.h"
#include "file_pool.h"
#include "thread.h"

#include <fstream>

typedef list<DataBlock_t*>  PACKET_BLOCK_LIST;

#define OUT_RESOUCE_COUNT  3

struct PacketMeta_t;
class CBlockCaptureImpl;
class CBlockCapture;
struct CaptureContext_t
{
    CBlockCapture*  captureObj;
    int             captureId;      // always a thread
    int             portId;         // which ports the thread will handle, such as 0x3 means port 0 and 1
};

class CBlockCapture : public CNetCapture
{
public:
    CBlockCapture();
    ~CBlockCapture();

    virtual int  startCapture(CaptureConfig_t& _config);
    virtual int  stopCapture();
    virtual bool openDevice(int _index, const char* _devName);
    virtual int  closeDevice();

    virtual int releaseBlock(DataBlock_t* _block);

protected:
    /**
    * @Function: called in the capture thread
    *  its main job is to copy the captured packets in a block share memory
    *  and put the full block, or by a timer flush, into a list.
    * @Memo:
    *   Since multiple thread may call this, _captureId specify the thread
    **/
    virtual bool openDevice_(int _index, const char* _devName) = 0;
    virtual bool closeDevice_() = 0;

    virtual int handlePacket(int _captureId, int _portIndex, packet_header_t* _header, const byte* _data, bool _rmon);
    virtual int handleBlock(DataBlock_t* _block, int _blockCount = 1);
    virtual int prepareResource(CaptureConfig_t& _config);
    virtual int releaseResource();
    virtual int setCaptureFilter_(const Filter_t& _filter, int _port, const char* _rawFilter);
    virtual int startCapture_(void* _arg);
    virtual ICaptureFile* createCaptureFile();

    int calcRmon(int _portIndex, const packet_header_t* _header, PacketMeta_t* _metaInfo, const byte* _pktData);
    int testCapture(int _captureId, int _portIndex, packet_header_t* _pktHeader, byte* _pktData, bool _rmon);
    int getCaptureThreadCount() { return m_capThreadCount;  }
    void alignBlock(int _captureId, DataBlock_t *_dataBlock);
    int detectFinishIo(int _captureId);
    bool isCaptureStarted(int _captureId);

private:
    /**
    * @Function: called in the capture thread
    *  its main job is to fetch packets from hardware and keep them in memory
    **/
    static  int doTheCapture_(void *_obj);

    bool isDataBlockFull(int _captureId, packet_header_t *_header);
    void flushDataBlock(int _captureId);
    PacketMeta_t *copy2Block(int _captureId, int _portIndex,
                             packet_header_t* _header, const byte* _data);

protected:
    CBlockCaptureImpl*  m_myImpl;
    DataBlock_t*        m_bufBlock[MAX_CAPTURE_THREAD];             // the currently used block
    packet_header_t*    m_prevPacket[MAX_CAPTURE_THREAD];           // keep the last captured packet, while need to pad the block
                                                                    // and there is <= 24 (the size of packet header) space left in the block,
                                                                    // we will pad the data on the previous packet
    LiangZhu::CSimpleThread*    m_captureThread[MAX_CAPTURE_THREAD];// the thread to fetch packets from hardware

    int                 m_capThreadCount;
    Statistic_t         m_portStats[MAX_PORT_NUM];

    bool                m_isReady;
};


#endif
