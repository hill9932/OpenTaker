#include "block_file.h"

namespace LiangZhu
{
    CBlockFile::CBlockFile()
        : CHFile(false)
    {
        m_blockSize = 0;
    }

    CBlockFile::~CBlockFile()
    {
        flush();
    }

    int CBlockFile::saveData(const byte* _data, u_int32 _dataLen)
    {
        if (!_data) return -1;

        assert(_dataLen < BUFFER_SIZE);

    retry:
        char* dst = m_block + m_blockSize;
        if (m_blockSize + _dataLen > BUFFER_SIZE)    // need more space
        {
            flush();
            goto retry;
        }
        else
        {
            memcpy(dst, _data, _dataLen);
            m_blockSize += _dataLen;
        }

        return _dataLen;
    }

    int CBlockFile::saveData(const tchar* _format, ...)
    {
        if (!_format)  return -1;
        char* dst = m_block + m_blockSize;

        va_list vArgList;
        va_start(vArgList, _format);

    retry:
        int n = vsnprintf_t(dst, BUFFER_SIZE - m_blockSize, _format, vArgList);
        if (n == (int)BUFFER_SIZE - m_blockSize)    // need more space
        {
            m_block[m_blockSize + 1] = 0;
            flush();

            dst = m_block;
            goto retry;
        }
        else
        {
            m_blockSize += n;
        }

        va_end(vArgList);

        return n;
    }

    int CBlockFile::flush()
    {
        if (m_blockSize > 0)
        {
            write_w((const byte*)m_block, m_blockSize);
            m_blockSize = 0;
        }

        return 0;
    }

    int CBlockFile::close()
    {
        flush();
        return CHFile::close();
    }
}
