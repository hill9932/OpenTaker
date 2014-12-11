/**
 * @Function:
 *  receive format string and cache in a block until flush into the disk
 * @Memo:
 *  created by hill, 8/1/2014
 **/
#ifndef __HILUO_BLOCK_FILE_INCLUDE_H__
#define __HILUO_BLOCK_FILE_INCLUDE_H__

#include "common.h"
#include "file.h"

#define BUFFER_SIZE     ONE_MB

namespace LiangZhu
{
    class CBlockFile : public CHFile
    {
    public:
        CBlockFile();
        virtual ~CBlockFile();
        int saveData(const tchar* _format, ...);
        int saveData(const byte* _data, u_int32 _dataLen);
        int flush();
        int close();

    private:
        char    m_block[BUFFER_SIZE];
        u_int32 m_blockSize;
    };
}

#endif // __BLOCK_FILE_H__
