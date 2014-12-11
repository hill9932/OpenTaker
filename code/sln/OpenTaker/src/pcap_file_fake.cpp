#include "pcap_file_fake.h"


int CFakePcapFile::open(const tchar* _fileName, int _access, int _flag, bool _asyn, bool _directIO)
{
    m_fileName = _fileName;
    setFilePos(getHeaderSize());
    return 0;
}

int CFakePcapFile::read(byte* _data, int _dataLen, u_int64 _offset, void* _context)
{
    PER_FILEIO_INFO_t readBlock;
    bzero(&readBlock, sizeof(OVERLAPPED));
    readBlock.optType = FILE_READ;

    m_filePool->m_readCallback(this, &readBlock, readBlock.dataLen);
    return 0;
}

int CFakePcapFile::write(const byte* _data, int _dataLen, u_int64 _offset,
                         void* _context, void* _context2, u_int32 _flag)
{
    u_int64 offset = _offset == -1 ? m_offset : _offset;
    PER_FILEIO_INFO_t writeBlock;
    bzero(&writeBlock, sizeof(OVERLAPPED));

    writeBlock.buffer = const_cast<byte*>(_data);
    writeBlock.dataLen = _dataLen;
    writeBlock.Offset = offset;
    writeBlock.flag = _flag;

#ifdef WIN32
    writeBlock.OffsetHigh = offset >> 32;
#endif
    writeBlock.context = _context;      // the CBlockCaptureImpl object
    writeBlock.context2 = _context2;    // the DataBlock_t

    writeBlock.hFile = m_fileHandle;
    writeBlock.optType = FILE_WRITE;

    m_offset += _dataLen;

    m_filePool->m_writeCallback(this, &writeBlock, writeBlock.dataLen);

    return 0;
}