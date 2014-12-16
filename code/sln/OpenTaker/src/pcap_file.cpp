#include "pcap_file.h"
#include "global.h"
#include "file.h"

CPCAPFile::CPCAPFile()
{
    m_fileHandle    = INVALID_HANDLE_VALUE;
    m_fileView      = (handle_t)NULL;
    m_buf           = NULL;
    m_offset        = 0;
    m_fileSize      = g_env ? g_env->m_config.storage.fileSize : 0;
}

CPCAPFile::~CPCAPFile()
{
    close();
}

int CPCAPFile::open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO)
{
    close();
    m_fileName = _fileName;

    int z = createMapFile(_fileName, (_access & ACCESS_WRITE) ? m_fileSize : 0);
    if (z != 0) return z;

    if (_access & ACCESS_READ)
    {
        m_fileSize = getSize();
        return 0;
    }

    return (_access & ACCESS_WRITE) ? writeHeader() : 0;
}

int CPCAPFile::writeHeader()
{
    if (!isValid()) return -1;
    m_offset = 0;
    return write((byte*)&PCAP_FILE_HEADER, sizeof(PCAP_FILE_HEADER), 0);
}

int CPCAPFile::write(const byte* _data, int _dataLen, u_int64 _offset, void* _context, void* _context2, u_int32 _flag)
{
    if (!_data || !isValid())   return -1;
    u_int64 offset = (_offset == (u_int64)-1) ? m_offset : (m_offset = _offset, _offset);
    if (_dataLen + offset > m_fileSize)    return -1;

    memcpy(m_buf + offset, _data, _dataLen);
    m_offset += _dataLen;

    return 0;
}

int CPCAPFile::read (byte* _data, int _dataLen, u_int64 _offset, void* _context, void* _context2)
{
    if (!_data || !isValid())       return -1;
    if ((u_int64)_dataLen > m_fileSize)      return -1;

    memcpy(_data, m_buf + _offset, _dataLen);

    return 0;
}

u_int32 CPCAPFile::getSize(u_int32* _high32)
{
    u_int32 high32 = 0;
    u_int32 low32 = 0;

#ifdef WIN32
    low32 = GetFileSize(m_fileHandle, (LPDWORD)&high32);
#else
    struct stat statbuf;
    ON_ERROR_LOG_LAST_ERROR_AND_DO(fstat(m_fileHandle, &statbuf), ==, -1, return 0);

    if (S_ISREG(statbuf.st_mode))
    {
        if (_high32)    *_high32 = statbuf.st_size >> 32;
        return statbuf.st_size;
    }
#endif

    if (_high32)    *_high32 = high32;
    return low32;
}

int CPCAPFile::rename(const tchar* _newFileName)
{
    if (!_newFileName || !isValid())  return -1;
    if (m_fileName == _newFileName)     return 0;

    int z = ::rename_t(m_fileName, _newFileName);
    ON_ERROR_LOG_LAST_ERROR(z, !=, 0);
    if (0 == z)
        m_fileName = _newFileName;
    return z;
}

#ifdef WIN32
#include "pcap_file_win32.hxx"
#else
#include "pcap_file_linux.hxx"
#endif

