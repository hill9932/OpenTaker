/**
 * @Function:
 *  This file wrap the interaction with sqlite db.
 * @Memo:
 *  Created by hill, 4/16/2014
 **/

#ifndef __HILUO_SQLITE_INCLUDE_H__
#define __HILUO_SQLITE_INCLUDE_H__

#include "common.h"
#include "sqlite3/sqlite3.h"

typedef int (*DB_CALLBACK)(void*, int, char**, char**);

namespace LiangZhu
{
    /**
     * @Function: transfer between two database.
     **/
    int SQLite3Backup(sqlite3* src_db, sqlite3* dest_db);

    /**
     * 1. Create the object using sqlite3_prepare_v2() or a related function.
     * 2. Bind values to host parameters using the sqlite3_bind_*() interfaces.
     * 3. Run the SQL by calling sqlite3_step() one or more times.
     * 4. Reset the statement using sqlite3_reset() then go back to step 2. Do this zero or more times.
     * 5. Destroy the object using sqlite3_finalize().
     **/
    class CSqlLiteDB
    {
    public:
        CSqlLiteDB();
        ~CSqlLiteDB();

        int  open(const tchar* _db_path);
        int  close();
        int  compile(const tchar* _tableName, const tchar* _sql);

        /**
         * @Function: execute the sql statement.
         * @Param _fmt: the format string, support %d (double), %u (numeric), %s (string)
         * @Param ...: the values
         * @Return:
         *  -1  failed
         *  -2  invalid format
         **/
        int  doStmt(const tchar* _tableName, const tchar* _fmt, ...);
        int  exec(const tchar* _sql, void* _context, DB_CALLBACK _callback = NULL);
        int  beginTransact();
        int  commit();
        int  flush();

        /**
         * @Return 1: exist
         *         0: not exist
         *        -1: error
         **/
        int  isTableExist(const tchar* _tableName);

        sqlite3_stmt* getStatment(const tchar* _tableName);
        operator sqlite3* ()    { return m_db; }
        CStdString getDbPath()  { return m_dbPath; }
        bool isValid()          { return m_db != NULL; }

    private:
        CStdString      m_dbPath;
        sqlite3*        m_db;
        map<CStdString, sqlite3_stmt*>   m_stmts;   // every table has a statement
        bool            m_hasTransaction;
    };

}

#endif
