/**
 * @Function:
 *  encapsulate libpq to operator postgresSQL
 * @Memo:
 *  Create by hill, 6/27/2014
 **/
#ifndef __HILUO_POSTGRES_DB_INCLUDE_H__
#define __HILUO_POSTGRES_DB_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    class CPostgresSQL
    {
    public:
        CPostgresSQL();
        ~CPostgresSQL();

        bool connect(const tchar* _connString);
        bool close();
        bool isConnect()    { return m_isConnect; }
        void* getHandle()   { return m_conn; }

        /**
         * @Function: judge whether a table has been create.
         * @Return 1: exist
         *         0: no-exist
         *         -1: error
         **/
        int isTableExist(const tchar* _tableName);
        int createTable(const tchar* _tableName, int _count, const tchar** _colNames, const tchar** _colType, const int* _colSize);
        int exec(const tchar* _sql);
        int beginCopy(const tchar* _sql);
        int copy(const char* _data, int _len);
        int endCopy();

    private:
        void *m_conn;
        void *m_res;
        bool m_isConnect;
    };
}

#endif