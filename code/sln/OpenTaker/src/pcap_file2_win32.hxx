#ifdef WIN32


int CPCAPFile2::read(byte* _data, int _dataLen, u_int32 _offset, void* _context)
{
    if (!m_filePool || !_data || _dataLen < 0)    return -1;

    PPER_FILEIO_INFO_t readBlock = m_filePool->getBlock();
    bzero(readBlock, sizeof(OVERLAPPED));
    readBlock->overlapped.Offset       = _offset == -1 ? getOffset() : _offset;
    readBlock->overlapped.OffsetHigh   = 0;
    readBlock->buffer   = _data;
    readBlock->dataLen  = _dataLen;
    readBlock->context  = _context;

    int z = CHFile::read(readBlock);
    if (0 != z)
        m_filePool->release(readBlock);

    return z;
}

int CPCAPFile2::write(const byte* _data, int _dataLen, u_int32 _offset, void* _context)
{
    if (!m_filePool || _dataLen < 0)    return -1;
    u_int32 offset = _offset == -1 ? m_offset : _offset;

    PPER_FILEIO_INFO_t writeBlock = m_filePool->getBlock();
    bzero(writeBlock, sizeof(OVERLAPPED));
    writeBlock->overlapped.Offset       = offset;
    writeBlock->overlapped.OffsetHigh   = 0;
    writeBlock->buffer  = const_cast<byte*>(_data);
    writeBlock->dataLen = _dataLen;
    writeBlock->context = _context;

    int z = CHFile::write(writeBlock);
    if (0 != z)
        m_filePool->release(writeBlock);
    else
        m_offset += _dataLen;

    return z;
}


#endif
