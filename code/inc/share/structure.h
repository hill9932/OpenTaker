/**
 * @Function: This file defines device related structures.
 **/
#ifndef __VP_STRUCTURE_INCLUDE_H__
#define __VP_STRUCTURE_INCLUDE_H__

#include "common.h"

enum PacketType_e
{
    PACKET_UNKNOWN  = 0,
    PACKET_PCAP     = 1,
    PACKET_NAPATECH = 2,
    PACKET_ACCOLADE = 3,
};

enum FileType_e
{
    FILE_PCAP       = 1,
    FILE_ACCOLADE   = 2,
    FILE_PV         = 3,
    FILE_FAKE
};


#define DATA_BLOCK_SIZE ONE_MB

struct BlockDesc_t
{
    u_int64         firstPacketTime;
    u_int64         lastPacketTime;
    u_int64         offset;         // offset in the file
    u_int32         blockSize;
    u_int32         usedSize;
    u_int32         pktCount;       // real packet
    u_int16         portNum;
};

struct DataBlockDesc_t : public BlockDesc_t
{
    u_int64         id;             // the index be requested

    u_int32         fileIndex;      // specify which file this block belonged to
    u_int32         metaBlkStartIdx;
    u_int32         metaBlkStopIdx;

    u_int32         nodeId;         // the numa node id this block memory belong to
    u_int32         captureId;      // the thread id to request this block, should in [0, 3]

    PacketType_e    type;           // PACKET_NORMAL / PACKET_ACCOLADE
    u_int16         resIndex;       // index the OutRes_t
    u_int16         target;
    byte*           pData;          // point to the to be saved data

    void*           capObject;      // CBlockCapture* object
    void*           param1;         
    void*           param2;         
};

struct DataBlockHeader_t
{
    union
    {
        DataBlockDesc_t dataDesc;
        byte pad[SECTOR_ALIGNMENT];
    };
};

struct  DataBlock_t : public DataBlockHeader_t
{
    byte    data[DATA_BLOCK_SIZE];
};

struct timestamp_t
{
    u_int32    tv_sec;
    u_int32    tv_nsec;
};

struct packet_header_t
{
    struct timestamp_t ts;	    // time stamp
    u_int32 caplen;	            // length of portion present
    u_int32 len;	            // length this packet (off wire)
};

/**
* @Function: Used to create the file name
**/
enum FileStatus_e
{
    STATUS_NORMAL   = 0,
    STATUS_LOCKED   = 1,
    STATUS_ERROR    = 2,
    STATUS_CAPTURE  = 3,
    STATUS_SHOW     = 4,
    STATUS_DELETING = 5
};

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

#define PCAP_NSEC_MAGIC         0xa1b23c4d
#define PCAP_MSEC_MAGIC         0xa1b2c3d4

#define PCAP_VERSION_MAJOR  2
#define PCAP_VERSION_MINOR  4

#endif

