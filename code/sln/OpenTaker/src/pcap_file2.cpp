#include "pcap_file2.h"
#include "global.h"
#include "capture.h"


CPCAPFile2::CPCAPFile2(CFilePool* _filePool, bool _secAlign)
: CHFile(_secAlign)
{
    m_offset    = 0;
    m_fileSize  = g_env->m_config.storage.fileSize;
    m_filePool  = _filePool;

#ifdef WIN32
    m_fileHeaderBuf = (byte*)malloc(SECTOR_ALIGNMENT);
#else
    posix_memalign((void**)&m_fileHeaderBuf, SECTOR_ALIGNMENT, SECTOR_ALIGNMENT);
#endif

    bzero(m_fileHeaderBuf, SECTOR_ALIGNMENT);
    memcpy(m_fileHeaderBuf, (byte*)&PCAP_FILE_HEADER, sizeof(PCAP_FILE_HEADER));
    m_secAlign = _secAlign;
}

CPCAPFile2::~CPCAPFile2()
{
    if (m_fileHeaderBuf)    free(m_fileHeaderBuf);
}

int CPCAPFile2::open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO)
{
    close();
    m_fileName = _fileName;
    m_offset = 0;

    int z = CHFile::open( _access,
                          _flag,
                          _asyn,
                          _directIO);
    if (z != 0) return z;

    //z = setFileSize(m_fileSize);

#ifdef WIN32
    if (_asyn)  z = m_filePool->attach(m_fileHandle);   // Linux will attach the event fd
#endif

    return (_access & ACCESS_WRITE) ? writeHeader() : 0;
}

int CPCAPFile2::writeHeader()
{
    if (!isValid()) return -1;
    m_offset = 0;
    int  len = sizeof(PCAP_FILE_HEADER);

    if (m_secAlign)
    {
        packet_header_t fakeHeader;
        bzero(&fakeHeader, sizeof(packet_header_t));
        fakeHeader.len = fakeHeader.caplen = SECTOR_ALIGNMENT - len - sizeof(packet_header_t);

        memcpy(m_fileHeaderBuf + len, &fakeHeader, sizeof(packet_header_t));
        len = SECTOR_ALIGNMENT;
    }

    return write(m_fileHeaderBuf, len, m_offset);
}

int CPCAPFile2::close()
{
    m_offset = 0;
    return CHFile::close();
}


int CPCAPFile2::read(byte* _data, int _dataLen, u_int64 _offset, void* _context, void* _context2)
{
    if (!_data || _dataLen < 0)    return -1;
    u_int64 offset = _offset == -1 ? m_offset : _offset;

    PPER_FILEIO_INFO_t readBlock = m_filePool->getBlock();
    bzero(readBlock, sizeof(OVERLAPPED));

    readBlock->buffer   = _data;
    readBlock->dataLen  = _dataLen;
    readBlock->context  = _context;
    readBlock->context2 = _context2;
    readBlock->Offset   = (u_int32)offset;
#ifdef WIN32
    readBlock->OffsetHigh = offset >> 32;
#endif
    int z = CHFile::read(readBlock);
    if (0 != z)
        m_filePool->release(readBlock);

    return z;
}


int CPCAPFile2::write(const byte* _data, int _dataLen, u_int64 _offset, void* _context, void* _context2, u_int32 _flag)
{
    if (_dataLen < 0)    return -1;

    int z = 0;
    u_int64 offset = _offset == -1 ? m_offset : _offset;
    assert(offset <= g_env->m_config.storage.fileSize);

    PPER_FILEIO_INFO_t writeBlock = m_filePool->getBlock();
    bzero(writeBlock, sizeof(OVERLAPPED));

    writeBlock->buffer  = const_cast<byte*>(_data);
    writeBlock->dataLen = _dataLen;
    writeBlock->Offset  = (u_int32)offset;
    writeBlock->flag    = _flag;

#ifdef WIN32
    writeBlock->OffsetHigh = offset >> 32;
#endif
    writeBlock->context = _context;
    writeBlock->context2 = _context2;

    z = CHFile::write(writeBlock);
    if (0 != z)
        m_filePool->release(writeBlock);
    else
        m_offset += _dataLen;

    return z;
}

int CPCAPFile2::write(int _ioCount, const byte* _data[], int _dataLen[], u_int64 _offset[],
                      void* _context, void* _context2[], u_int32 _flag)
{
    assert(_ioCount < 512);
    if (_ioCount <= 0 || _ioCount > 512)    return -1;

    PPER_FILEIO_INFO_t requests[512] = { NULL };
    int z = 0;
    for (int i = 0; i < _ioCount; ++i)
    {
        u_int64 offset = _offset[i] == -1 ? m_offset : _offset[i];
        assert(offset <= g_env->m_config.storage.fileSize);

        PPER_FILEIO_INFO_t writeBlock = m_filePool->getBlock();
        bzero(writeBlock, sizeof(OVERLAPPED));

        writeBlock->buffer  = const_cast<byte*>(_data[i]);
        writeBlock->dataLen = _dataLen[i];
        writeBlock->flag    = _flag;
        writeBlock->Offset  = (u_int32)offset;
        writeBlock->context = _context;
        writeBlock->context2 = _context2[i];        
        writeBlock->hFile = m_fileHandle;
        writeBlock->optType = FILE_WRITE;
        writeBlock->index = m_writeIndex++;
#ifdef WIN32
        writeBlock->OffsetHigh = offset >> 32;
#else
        io_prep_pwrite(writeBlock, m_fileHandle, writeBlock->buffer, writeBlock->dataLen, writeBlock->Offset);
#endif

        m_offset += _dataLen[i];
        requests[i] = writeBlock;
    }

    z = CHFile::write(requests, _ioCount);

    return z;
}



