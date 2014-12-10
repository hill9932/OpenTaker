#ifndef __HILUO_FILE_INCLUDE_H__
#define __HILUO_FILE_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    enum OPERATION_TYPE
    {
        FILE_READ = 1,
        FILE_WRITE
    };

    typedef struct OVERLAPIOINFO_t : public OVERLAPPED
    {
        handle_t        hFile;
        OPERATION_TYPE  optType;
        u_int32         index;
        u_int32         dataLen;
        byte*           buffer;
        void*           context;
        void*           context2;
        u_int32         flag;
    } PER_FILEIO_INFO_t, *PPER_FILEIO_INFO_t;


    enum FILE_ACCESS
    {
        ACCESS_NONE = 0,
        ACCESS_READ = 1,
        ACCESS_WRITE = 2
    };

    enum FILE_CREATE
    {
        FILE_CREATE_NONE = 0,
        FILE_CREATE_NEW = 1,
        FILE_CREATE_ALWAYS = 2,
        FILE_OPEN_EXISTING = 4,
        FILE_OPEN_ALWAYS = 8,
        FILE_TRUNCATE_EXISTING = 0x10
    };

    class CFilePool;
    class CHFile
    {
    public:

        CHFile(bool _secAlign = true);
        CHFile(CStdString& _fileName, bool _autoClose = true, bool _secAlign = true);
        virtual ~CHFile();

        bool isValid() { return m_fileHandle != INVALID_HANDLE_VALUE; }

        void setFileName(const tchar* _fileName) { close();  m_fileName = _fileName; }
        void setFileName(CStdString& _fileName)  { setFileName(_fileName.c_str()); }
        CStdString getFileName()                 { return m_fileName; }

        virtual int  open(int _access, int _flag, bool _asyn = true, bool _directIO = false);
        virtual int  close();

        /**
         * @Function: use aio to read/write data
         **/
        int     read(PPER_FILEIO_INFO_t _ioRequest);
        int     write(PPER_FILEIO_INFO_t _ioRequest);
        int     write(PPER_FILEIO_INFO_t _ioRequests[], int _count);

        /**
         * @Function: use sync io, _w means wait
         **/
        int     read_w(byte* _data, int _dataLen, u_int64 _offset = -1);
        int     write_w(const byte* _data, int _dataLen, u_int64 _offset = -1);

        void attach(handle_t _fileHandle, bool _autoClose = true);
        void setFilePool(CFilePool* _filePool) { m_filePool = _filePool; }

        u_int32 getSize(u_int32* _high32 = NULL);
        int     setSize(u_int64 _fileSize);
        int     rename(const tchar* _newFileName);
        void    seek(u_int64 _offset);
        u_int64 getPos();

        operator handle_t() { return m_fileHandle; }
        DEFINE_EXCEPTION(Init);

    private:
        bool init();

    protected:
        CFilePool*  m_filePool;     // for anyn io

        CStdString  m_fileName;
        handle_t    m_fileHandle;
        bool        m_autoClose;

        u_int32     m_readIndex;
        u_int32     m_writeIndex;

        bool        m_directIO;
        bool        m_secAlign;
    };

}

#endif
