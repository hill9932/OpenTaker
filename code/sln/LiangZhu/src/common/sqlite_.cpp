#include "sqlite_.h"
#include "file_system.h"
#include "string_.h"

namespace LiangZhu
{
    int SQLite3Backup(sqlite3* src_db, sqlite3* dest_db)
    {
        int rc = SQLITE_ERROR;
        sqlite3_backup* db_backup = sqlite3_backup_init(dest_db, "main", src_db, "main");
        if (NULL != db_backup)
        {
            rc = sqlite3_backup_step(db_backup, -1);
            rc = sqlite3_backup_finish(db_backup);
        }
        return rc;
    }

    /*
    int sqliteEscape(CStdString& keyWord)
    {
    keyWord.replace("/", "//");
    keyWord = keyWord.replace("'", "''");
    keyWord = keyWord.replace("[", "/[");
    keyWord = keyWord.replace("]", "/]");
    keyWord = keyWord.replace("%", "/%");
    keyWord = keyWord.replace("&", "/&");
    keyWord = keyWord.replace("_", "/_");
    keyWord = keyWord.replace("(", "/(");
    keyWord = keyWord.replace(")", "/)");
    return keyWord;
    }
    */

    CSqlLiteDB::CSqlLiteDB()
    {
        m_db = NULL;
        m_hasTransaction = false;
    }

    CSqlLiteDB::~CSqlLiteDB()
    {
        close();
    }

    int CSqlLiteDB::open(const tchar* _dbPath)
    {
        if (!_dbPath)  return -1;

        RM_LOG_TRACE("Open db: " << _dbPath);

        close();
        int rc = sqlite3_open(_dbPath, &m_db);
        if (rc)
        {
            RM_LOG_ERROR("Can't open database: " << sqlite3_errmsg(m_db));
            sqlite3_close(m_db);
            m_db = NULL;
        }
        else
            m_dbPath = _dbPath;

        return rc;
    }

    int CSqlLiteDB::close()
    {
        int z = 0;
        if (m_db)
        {
            RM_LOG_TRACE("Close db: " << m_dbPath);

            map<CStdString, sqlite3_stmt*>::iterator it = m_stmts.begin();
            for (; it != m_stmts.end(); ++it)
            {
                sqlite3_stmt* stmt = it->second;
                if (!stmt)  continue;

                sqlite3_finalize(stmt);
            }
            commit();
            m_stmts.clear();

            z = sqlite3_close(m_db);
            m_db = NULL;
        }

        return z;
    }

    sqlite3_stmt* CSqlLiteDB::getStatment(const tchar* _tableName)
    {
        if (!_tableName)    return NULL;
        if (!m_stmts.count(_tableName)) return NULL;
        return m_stmts[_tableName];
    }


    int CSqlLiteDB::compile(const tchar* _tableName, const tchar* _sql)
    {
        if (!m_db || !_sql)  return -1;
        sqlite3_stmt* stmt = getStatment(_tableName);
        if (stmt)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }

        RM_LOG_TRACE("Compile on " << m_dbPath << ": " << _sql);

        int z = sqlite3_prepare_v2(m_db, _sql, -1, &stmt, NULL);
        if (0 != z)
        {
            RM_LOG_ERROR(sqlite3_errmsg(m_db));
        }
        else
        {
            m_stmts[_tableName] = stmt;
        }

        return z;
    }

    int CSqlLiteDB::doStmt(const tchar* _tableName, const tchar* _fmt, ...)
    {
        if (!m_db || !_fmt)  return -1;

        sqlite3_stmt* stmt = getStatment(_tableName);
        if (!stmt)  return -1;

        int z = 0;
        z = sqlite3_reset(stmt);
        if (0 != z)
            RM_LOG_ERROR(sqlite3_errmsg(m_db));

        va_list args;
        va_start(args, _fmt);

        int n = 0;
        vector<CStdString> vec;
        StrSplit(_fmt, ",", vec);

        for (unsigned int i = 0; i < vec.size(); ++i)
        {
            ++n;
            if (vec[i] == "%d")
            {
                double v = va_arg(args, double);
                z = sqlite3_bind_double(stmt, n, v);
                ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, sqlite3_errmsg(m_db), return -2);
            }
            else if (vec[i] == "%u")
            {
                int v = va_arg(args, int);
                z = sqlite3_bind_int(stmt, n, v);
                ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, sqlite3_errmsg(m_db), return -2);

            }
            else if (vec[i] == "%s")
            {
                char* v = va_arg(args, char*);
                z = sqlite3_bind_text(stmt, n, v, strlen(v), NULL);
                ON_ERROR_LOG_MESSAGE_AND_DO(z, != , 0, sqlite3_errmsg(m_db), return -2);
            }
            else
            {
                return -2;
            }
        }

        z = sqlite3_step(stmt);
        if (0 != z && SQLITE_DONE != z)
            RM_LOG_ERROR(sqlite3_errmsg(m_db));

        return z;
    }

    int CSqlLiteDB::exec(const tchar* _sql, void* _context, DB_CALLBACK _callback)
    {
        if (!m_db || !_sql)  return -1;

        RM_LOG_TRACE("Do sql on " << m_dbPath << ": " << _sql);

        char *errMsg = NULL;
        int z = sqlite3_exec(m_db, _sql, _callback, _context, &errMsg);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , SQLITE_OK, m_dbPath << ": " << errMsg, { if (errMsg) sqlite3_free(errMsg); });

        return z;
    }

    int CSqlLiteDB::beginTransact()
    {
        if (!m_db)  return -1;

        RM_LOG_TRACE("Begin transaction: " << m_dbPath);

        char *errMsg = NULL;
        int z = sqlite3_exec(m_db, "begin;", 0, 0, &errMsg);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , SQLITE_OK, m_dbPath << ": " << errMsg, { if (errMsg) sqlite3_free(errMsg); });

        m_hasTransaction = true;
        return z;
    }

    int CSqlLiteDB::commit()
    {
        if (!m_db)  return -1;
        if (!m_hasTransaction)  return 0;

        RM_LOG_TRACE("Commit transaction: " << m_dbPath);

        char *errMsg = NULL;
        int z = sqlite3_exec(m_db, "commit;", NULL, NULL, &errMsg);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , SQLITE_OK, m_dbPath << ": " << errMsg, { if (errMsg) sqlite3_free(errMsg); });

        m_hasTransaction = false;
        return z;
    }

    int CSqlLiteDB::flush()
    {
        commit();
        return beginTransact();
    }

    static int TableExist_Callback(void* _exist, int _argc, char** _argv, char** _azColName)
    {
        for (int i = 0; i < _argc; i++)
        {
            if (_exist)
            {
                *(bool*)_exist = atoi(_argv[0]) == 1;
                break;
            }
        }

        return 0;
    }

    int CSqlLiteDB::isTableExist(const tchar* _tableName)
    {
        if (!m_db || !_tableName)   return false;
        CStdStringA sql = "SELECT COUNT(*) FROM SQLITE_MASTER WHERE type='table' AND name='";
        sql += _tableName;
        sql += "'";

        char *errMsg = NULL;
        bool isExist = false;
        int z = sqlite3_exec(m_db, sql, TableExist_Callback, &isExist, &errMsg);
        ON_ERROR_LOG_MESSAGE_AND_DO(z, != , SQLITE_OK, errMsg, sqlite3_free(errMsg));
        return z == SQLITE_OK ? isExist : -1;
    }

}
