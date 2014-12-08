#include "file.h"
#include "file_pool.h"

namespace LiangZhu
{
    CHFile::CHFile(bool _secAlign)
    {
        m_filePool = NULL;
        m_secAlign = _secAlign;
        m_autoClose = true;
        init();
    }

    CHFile::CHFile(CStdString& _fileName, bool _autoClose, bool _secAlign)
        : m_fileName(_fileName)
    {
        if (m_fileName.empty())
            throw "Invalid file name.";

        m_secAlign = _secAlign;
        m_autoClose = _autoClose;
        init();
    }

    bool CHFile::init()
    {
        m_fileHandle = INVALID_HANDLE_VALUE;
        m_readIndex = m_writeIndex = 0;

        return true;
    }

    CHFile::~CHFile()
    {
        if (m_autoClose)
        {
            close();
        }
    }


    int CHFile::close()
    {
        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
        }

        m_fileName.Empty();
        return 0;
    }

    u_int32 CHFile::getSize(u_int32* _high32)
    {
        u_int32 high32 = 0;
        u_int32 low32 = 0;

#ifdef WIN32
        low32 = GetFileSize(m_fileHandle, (LPDWORD)&high32);
#else
        struct stat statbuf;
        if (fstat(m_fileHandle, &statbuf) == -1)
        {
            return 0;
        }

        if(S_ISREG(statbuf.st_mode))    return statbuf.st_size;
#endif

        if (_high32)    *_high32 = high32;
        return low32;
    }

    int CHFile::setSize(u_int64 _fileSize)
    {
        int z = 0;

#ifdef WIN32
        LONG    hightSize = _fileSize >> 32;
        u_int32 newSize = SetFilePointer(m_fileHandle, (LONG)_fileSize, &hightSize, FILE_BEGIN);

        if (!SetEndOfFile(m_fileHandle))
            z = GetLastSysError();
#else
        z = truncate(m_fileName, _fileSize);
#endif

        if (0 != z) LOG_ERROR_MSG(z);

        return z;
    }

    void CHFile::seek(u_int64 _offset)
    {
#ifdef WIN32
        LONG highOffset = _offset >> 32;
        SetFilePointer(m_fileHandle, (LONG)_offset, &highOffset, FILE_BEGIN);
#else
        lseek64(m_fileHandle, _offset, SEEK_SET);
#endif
    }

    u_int64 CHFile::getPos()
    {
#ifdef WIN32
        return SetFilePointer(m_fileHandle, 0, NULL, FILE_CURRENT);
#else
        return lseek64(m_fileHandle, 0, SEEK_SET);
#endif
    }

    int CHFile::rename(const tchar *_newFileName)
    {
        if (!_newFileName || !isValid())  return -1;
        if (m_fileName == _newFileName) return 0;

        int z = ::rename_t(m_fileName, _newFileName);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, _newFileName, return err);
        if (z == 0)
            m_fileName = _newFileName;

        return z;
    }

    void CHFile::attach(handle_t _fileHandle, bool _autoClose /* = true */)
    {
        m_fileHandle = _fileHandle;
        m_autoClose = _autoClose;
    }

    int CHFile::read(PPER_FILEIO_INFO_t _ioRequest)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE)  return -1;

        _ioRequest->hFile = m_fileHandle;
        _ioRequest->optType = FILE_READ;
        _ioRequest->index = m_readIndex;

#ifdef WIN32
        DWORD dataRead = 0;
        if (ReadFile(m_fileHandle,
            _ioRequest->buffer,
            _ioRequest->dataLen,
            &dataRead,
            (LPOVERLAPPED)_ioRequest) ||
            ERROR_IO_PENDING == GetLastSysError())

#else
        struct iocb* iocbs[1] = {0};
        iocbs[0] = _ioRequest;
        io_prep_pread(_ioRequest, m_fileHandle, _ioRequest->buffer, _ioRequest->dataLen, _ioRequest->Offset);

        int z = io_submit(m_filePool->getAioObject(), 1, iocbs);
        if (z == 1)
#endif
        {
            ++m_readIndex;
        }
        else
        {
            int err = GetLastSysError();
            LOG_ERROR_MSG(err);
            return err;
        }

        return 0;
    }


    int CHFile::write(PPER_FILEIO_INFO_t _ioRequest)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE || !_ioRequest)  return -1;

        _ioRequest->hFile = m_fileHandle;
        _ioRequest->index = m_writeIndex;
        _ioRequest->optType = FILE_WRITE;

#ifdef WIN32
        if (WriteFile(m_fileHandle,
            _ioRequest->buffer,
            _ioRequest->dataLen,
            NULL,
            (LPOVERLAPPED)_ioRequest) ||
            ERROR_IO_PENDING == GetLastSysError())
#else

        struct iocb* iocbs = _ioRequest;
        io_prep_pwrite(_ioRequest, m_fileHandle, _ioRequest->buffer, _ioRequest->dataLen, _ioRequest->Offset);

        int z = io_submit(m_filePool->getAioObject(), 1, &iocbs);
        if (z != 1) SetLastSysError(-z);

        if (z == 1)
#endif
        {
            ++m_writeIndex;
        }
        else
        {
            int err = GetLastSysError();
            LOG_ERROR_MSG(err);
            return err;
        }

        return 0;
    }

    int CHFile::write(PPER_FILEIO_INFO_t _ioRequests[], int _count)
    {
        if (m_filePool->getAioObject() == 0)    return -1;

        assert(_count < 512);

        int z = 0;
#ifdef LINUX
        /*   struct iocb* iocbs[512] = {0};

           for (int i = 0; i < _count; ++i)
           {
           #ifdef LINUX
           iocbs[i] = _ioRequests[i];
           #endif
           }
           */

        z = io_submit(m_filePool->getAioObject(), _count, (iocb**)_ioRequests);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(z, !=, _count, SetLastSysError(-z));

#else
        for (int i = 0; i < _count; ++i)
            z = write(_ioRequests[i]);

#endif

        return z;
    }


#ifdef WIN32

    int CHFile::open(int _access, int _flag, bool _asyn, bool _directIO)
    {
        int access = ACCESS_NONE;
        if (_access & ACCESS_READ)  access |= GENERIC_READ;
        if (_access & ACCESS_WRITE) access |= GENERIC_WRITE;
        if (access == ACCESS_NONE)  return -1;

        int flag = FILE_CREATE_NONE;
        if (_flag & FILE_CREATE_NEW)        flag |= CREATE_NEW;
        if (_flag & FILE_CREATE_ALWAYS)     flag |= CREATE_ALWAYS;
        if (_flag & FILE_OPEN_ALWAYS)       flag |= OPEN_ALWAYS;
        if (_flag & FILE_OPEN_EXISTING)     flag |= OPEN_EXISTING;
        if (_flag & FILE_TRUNCATE_EXISTING) flag |= TRUNCATE_EXISTING;

        if (flag == FILE_CREATE_NONE)  return -1;
        int opt = _asyn ? FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED : FILE_ATTRIBUTE_NORMAL;
        if (_directIO)  opt |= FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING;

        m_fileHandle = CreateFile(m_fileName,
            access,          // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_DELETE, // dwShareMode
            NULL,            // lpSecurityAttributes
            flag,            // dwCreationDisposition
            opt,             // dwFlagsAndAttributes
            NULL);           // hTemplate
        ON_ERROR_LOG_MESSAGE_AND_DO(m_fileHandle, == , INVALID_HANDLE_VALUE, m_fileName, return err);

        m_directIO = _directIO;
        return 0;
    }

    int CHFile::read_w(byte* _data, int _dataLen, u_int64 _offset)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE)  return -1;

        if (_offset != -1)
        {
            LONG highOffset = _offset >> 32;
            SetFilePointer(m_fileHandle, (LONG)_offset, &highOffset, FILE_BEGIN);
        }

        DWORD   bytesRead = 0;
        int n = ReadFile(m_fileHandle, _data, _dataLen, &bytesRead, NULL);
        return bytesRead;
    }

    int CHFile::write_w(const byte* _data, int _dataLen, u_int64 _offset)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE || !_data)  return -1;
        if (_offset != -1)
        {
            LONG highOffset = _offset >> 32;
            SetLastSysError(0); // clear the remain flag
            SetFilePointer(m_fileHandle, (LONG)_offset, &highOffset, FILE_BEGIN);
            if (GetLastSysError() != 0)
                return 0;
        }

        DWORD bytesWritten = 0;
        int n = WriteFile(m_fileHandle, _data, _dataLen, &bytesWritten, NULL);
        ON_ERROR_LOG_LAST_ERROR(bytesWritten, != , _dataLen);
        return bytesWritten;
    }

#else // for linux

    int CHFile::open(int _access, int _flag, bool _asyn, bool _directIO)
    {
        int access = -1;
        if (_access & ACCESS_READ &&
            _access & ACCESS_WRITE)
            access = O_RDWR;
        else if (_access & ACCESS_WRITE)
            access = O_WRONLY;
        else if (_access & ACCESS_READ)
            access = O_RDONLY;

        if (access == -1)  return -1;

        if (_flag & FILE_OPEN_ALWAYS)       access |= O_CREAT;
        if (_flag & FILE_TRUNCATE_EXISTING) access |= O_TRUNC;
        if (_flag & FILE_CREATE_ALWAYS)     access |= O_TRUNC | O_CREAT;

        if (_directIO)  access |= O_DIRECT;
        if (_asyn)      access |= O_NONBLOCK;

        access |= O_LARGEFILE;

    retry:
        m_fileHandle = ::open(m_fileName, access, 0644);
        int err = GetLastSysError();

        ON_ERROR_LOG_MESSAGE_AND_DO(m_fileHandle, == , INVALID_HANDLE_VALUE, m_fileName, 1);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            if (err == 22)  // not support O_DIRECT
            {
                access &= ~O_DIRECT;
                goto retry;
            }
            return err;
        }

        m_directIO = _directIO;
        return 0;
    }

    int CHFile::read_w(byte* _data, int _dataLen, u_int64 _offset)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE)  return -1;

        if (_offset != (u_int64)-1)  lseek64(m_fileHandle, _offset, SEEK_SET);

        int n = ::read(m_fileHandle, _data, _dataLen);
        ON_ERROR_LOG_LAST_ERROR(n, == , -1);
        return n;
    }

    int CHFile::write_w(const byte* _data, int _dataLen, u_int64 _offset)
    {
        if (m_fileHandle == INVALID_HANDLE_VALUE || !_data)  return -1;
        if (_offset != (u_int64)-1)  lseek64(m_fileHandle, _offset, SEEK_SET);

        int n = ::write(m_fileHandle, _data, _dataLen);
        ON_ERROR_LOG_LAST_ERROR(n, != , _dataLen);
        return n;
    }

#endif

}
