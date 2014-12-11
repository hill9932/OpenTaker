/**
 * @Function:
 *  This file defines structures and function about pcap format file.
 * @Memo:
 *  Created by hill 4/16/2014
 **/
#ifndef __HILUO_PCAP_FILE_INCLUDE_H__
#define __HILUO_PCAP_FILE_INCLUDE_H__

#include "capture_file.h"


class CPCAPFile : public ICaptureFile
{
public:
    CPCAPFile();
    ~CPCAPFile();

    virtual FileType_e getFileType() { return FILE_PCAP; }
    virtual int open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO);
    virtual int close();
    virtual int read (  byte* _data, 
                        int _dataLen, 
                        u_int64 _offset = -1, 
                        void* _context = NULL, 
                        void* _context2 = NULL);

    virtual int write(  const byte* _data, 
                        int _dataLen, 
                        u_int64 _offset = -1,
                        void* _context = NULL, 
                        void* _context2 = NULL, 
                        u_int32 _flag=0);

    virtual int write(  int _ioCount, 
                        const byte* _data[], 
                        int _dataLen[], 
                        u_int64 _offset[],
                        void* _context = NULL, 
                        void* _context2[] = NULL, 
                        u_int32 _flag = 0) 
    {
        return -1;
    }

    virtual int rename(const tchar* _fileName);
    virtual int flush();
    virtual CStdString getFileName() { return m_fileName; }

    virtual bool isValid()          { return m_buf != NULL; }
    virtual u_int64 getOffset()     { return m_offset;      }
    virtual u_int64 getFileSize()   { return m_fileSize;    }
    virtual int setFileSize(u_int64 _fileSize);
    virtual int setFilePos(u_int64  _filePos) { m_offset = _filePos; return 0; }
    virtual int getHeaderSize()     { return sizeof(pcap_file_header_t); }
    virtual bool needPadding()      { return true; }

    byte* getBuf() { return m_buf;  }

private:
    int createMapFile(const tchar* _fileName, u_int64 _fileSize);

    /**
    * @Function: write the pcap file header
    **/
    int writeHeader();

    u_int32 getSize(u_int32* _high32 = NULL);

protected:
    CStdString  m_fileName;
    handle_t    m_fileHandle;
    handle_t    m_fileView;
    byte*       m_buf;
    u_int64     m_offset;
    u_int64     m_fileSize;
};

#endif
