/**
 * @Function
 *  Use block file as db file, every record has the same size and no index support
 **/
#ifndef __VP_RAW_DB_INCLUDE_H__
#define __VP_RAW_DB_INCLUDE_H__

#include "common.h"
#include "block_file.h"
#include "capture.h"

class CRawDB
{
public:
    CRawDB();
    ~CRawDB();

    int open(const tchar* _dbPath);
    int queryOnRecordDB(RecordFileFunc _func, void* _context);
    int updateOnRecordDB(RecordFileFunc _func, void* _context);

private:
    CBlockFile*  m_file;
};

#endif
