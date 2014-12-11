/**
 * @Function:
 *  Defines some interfaces for capturing
 * @Memo:
 *  Create by hill, 2014/5/5
 **/

#ifndef __HILUO_CAPTURE_FILE_INCLUDE_H__
#define __HILUO_CAPTURE_FILE_INCLUDE_H__

#include "common.h"
#include "structure.h"

//
// the file type supported
//
enum FileType_e
{
    FILE_PCAP = 1,
    FILE_ACCOLADE = 2,
    FILE_PV = 3,
    FILE_FAKE
};

//
// keep the information of every file
//
struct FileInfo_t
{
    int         index;
    u_int64     firstPacketTime;
    u_int64     lastPacketTime;
    int         age;
    int         status;
    u_int64     size;
    u_int64     packetCount;
    u_int64     bytesCount;
};

//
// pcap file header
//
#pragma pack(push, 4)

typedef struct pcap_file_header_t
{
    u_int32 magic;
    u_int16 version_major;
    u_int16 version_minor;
    u_int32 thiszone;
    u_int32 sigfigs;
    u_int32 snaplen;
    u_int32 linktype;
} pcap_file_header_t;

#pragma pack(pop)


#define PCAP_VERSION_MAJOR  2
#define PCAP_VERSION_MINOR  4

static const pcap_file_header_t PCAP_FILE_HEADER =
{
    0xA1B2C3D4,
    PCAP_VERSION_MAJOR,
    PCAP_VERSION_MINOR,
    0,
    0,
    0xFFFF,
    FILE_PCAP
};

/**
* @Function: Used to create the file name
**/
enum FileStatus_e
{
    STATUS_NORMAL = 0,
    STATUS_LOCKED = 1,
    STATUS_ERROR = 2,
    STATUS_CAPTURE = 3,
    STATUS_SHOW = 4,
    STATUS_DELETING = 5
};

struct Filter_t
{
    bool valid;
    bool isDrop;
    bool isSymm;
    bool isSrcHost;
    bool isDstHost;
    bool isSrcNet;
    bool isDstNet;
    bool isSrcPort;
    bool isDstPort;
    bool isSrcMac;
    bool isDstMac;
    bool isUDP;
    bool isTCP;

    u_int64 timestampMin;
    u_int64 timestampMax;
    u_int64 srcMac;
    u_int64 dstMac;
    u_int32 srcNet;
    u_int32 dstNet;
    u_int32 srcHost;
    u_int32 dstHost;
    u_int16 srcPortStart;
    u_int16 srcPortStop;
    u_int16 dstPortStart;
    u_int16 dstPortStop;
    u_int16 srcNetSize;
    u_int16 dstNetSize;
    u_int16 segment;
    u_int16 slice;
};

struct ICaptureFile
{
    virtual ~ICaptureFile() {}
    virtual FileType_e getFileType() = 0;

    virtual int open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO) = 0;
    virtual int close() = 0;
    virtual int read (  byte*   _data,
                        int     _dataLen,
                        u_int64 _offset = -1,
                        void*   _context = NULL,
                        void*   _context2 = NULL) = 0;

    virtual int write(  const byte* _data,
                        int         _dataLen,
                        u_int64     _offset = -1,
                        void*       _context = NULL,
                        void*       _context2 = NULL,
                        u_int32     _flag = 0) = 0;

    virtual int write(  int         _ioCount,
                        const byte* _data[],
                        int         _dataLen[],
                        u_int64     _offset[],
                        void*       _context = NULL,
                        void*       _context2[] = NULL,
                        u_int32     _flag = 0) = 0;

    virtual u_int64 getOffset() = 0;

    virtual int rename(const tchar* _fileName)  = 0;
    virtual int setFileSize(u_int64 _fileSize) = 0;
    virtual int setFilePos(u_int64 _filePos) = 0;
    virtual int getHeaderSize() = 0;
    virtual u_int64 getFileSize() = 0;
    virtual bool needPadding() = 0;
    virtual CStdString getFileName() { return ""; }

    virtual int flush() = 0;
    virtual bool isValid() = 0;
};

#endif
