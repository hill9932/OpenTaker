/**
 * @Function:
 *  Define the class derive from CPCAPFile, but will use direct asyn IO to save block,
 *  rather than memory mapping file.
 * @Memo:
 *  Create by hill, 5/5/2014
 **/

#ifndef __HILUO_PCAP_FILE2_INCLUDE_H__
#define __HILUO_PCAP_FILE2_INCLUDE_H__

#include "file.h"
#include "file_pool.h"
#include "capture_file.h"

class CPCAPFile2 : public CHFile, public ICaptureFile
{
public:
    CPCAPFile2(CFilePool* _filePool, bool _secAlign = true);
    ~CPCAPFile2();

    virtual FileType_e getFileType() { return FILE_PCAP; }
    virtual int open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO);
    virtual int close();
    virtual int read (byte* _data, int _dataLen, u_int64 _offset = -1, void* _context = NULL, void* _context2 = NULL);

    /**
     * @Function: use aio to write block data.
     * @Param _offset: the start position of the file to write.
     *                 -1 means append to the last position
     * @Param _context: currently it is the CBlockCaptureImpl*
     * @Param _context2: currently it is the DataBlock_t*
     **/
    virtual int write(  const byte* _data, 
                        int _dataLen, 
                        u_int64 _offset = -1,
                        void* _context = NULL, 
                        void* _context2 = NULL,
                        u_int32 _flag = 0);

    virtual int write(  int _ioCount, 
                        const byte* _data[], 
                        int _dataLen[], 
                        u_int64 _offset[],
                        void* _context = NULL, 
                        void* _context2[] = NULL, 
                        u_int32 _flag = 0);

    virtual int setFileSize(u_int64 _fileSize)
    {
        int z = CHFile::setSize(_fileSize);
        if (z == 0) m_fileSize = _fileSize;
        return z;
    }
    virtual int setFilePos(u_int64  _filePos)  { m_offset = _filePos;  return 0;    }
    virtual u_int64 getFileSize()              { return m_fileSize;                 }
    virtual int rename(const tchar* _fileName) { return CHFile::rename(_fileName);  }
    virtual int flush()                        { return 0;                          }
    virtual u_int64 getOffset()                { return m_offset;                   }
    virtual bool isValid()                     { return CHFile::isValid();          }
    virtual int getHeaderSize()                { return m_secAlign ? SECTOR_ALIGNMENT : sizeof(pcap_file_header_t); }
    virtual bool needPadding()                 { return true; }
    virtual CStdString getFileName()           { return m_fileName; }

private:
    /**
    * @Function: write the pcap file header
    **/
    virtual int writeHeader();

protected:
    u_int64     m_offset;
    u_int64     m_fileSize;
    byte*       m_fileHeaderBuf;    // [SECTOR_ALIGNMENT];
};

#endif
