/**
 * @Function:
 *  A fake file which will not exist on the disk.
 * @Memo:
 *  Create by hill, 7/5/2014
 **/

#ifndef __HILUO_PCAP_FILE_FAKE_INCLUDE_H__
#define __HILUO_PCAP_FILE_FAKE_INCLUDE_H__

#include "pcap_file2.h"

class CFakePcapFile : public CPCAPFile2
{
public:
    CFakePcapFile(CFilePool* _filePool, bool _secAlign) : CPCAPFile2(_filePool, _secAlign)
    { }

    virtual int open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO);
    virtual FileType_e getFileType() { return FILE_FAKE; }
    virtual bool needPadding()  { return false; }
    virtual bool isValid()      { return true;  }
    virtual int close()         { return 0;     }
    virtual int flush()         { return 0;     }
    virtual int rename(const tchar* _fileName) { return 0; }

    virtual int read(   byte* _data, 
                        int _dataLen, 
                        u_int64 _offset = -1, 
                        void* _context = NULL);

    virtual int write(  const byte* _data, 
                        int _dataLen, 
                        u_int64 _offset = -1, 
                        void* _context = NULL, 
                        void* _context2 = NULL, 
                        u_int32 _flag = 0);
};

#endif
