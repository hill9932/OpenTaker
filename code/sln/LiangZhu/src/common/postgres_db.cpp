#include "postgres_db.h"
#include "postgre/libpq-fe.h"

namespace LiangZhu
{
    CPostgresSQL::CPostgresSQL()
    {
        m_conn = NULL;
        m_res = NULL;
        m_isConnect = false;
    }

    CPostgresSQL::~CPostgresSQL()
    {
        close();
    }

    bool CPostgresSQL::connect(const tchar* _connString)
    {
        if (!_connString)   return false;
        close();

        m_conn = PQconnectdb(_connString);
        if (PQstatus((PGconn*)m_conn) == CONNECTION_OK)
        {
            //RM_LOG_INFO("Success to connect " << _connString);
        }
        else
        {
            RM_LOG_ERROR("Fail to connect " << _connString << ": " << PQerrorMessage((PGconn*)m_conn));
            return false;
        }

        m_isConnect = true;
        return true;
    }

    bool CPostgresSQL::close()
    {
        if (m_conn)
        {
            PQfinish((PGconn*)m_conn);
            m_conn = NULL;
        }

        m_isConnect = false;
        return true;
    }

    int CPostgresSQL::isTableExist(const char* _tableName)
    {
        CStdString sql = "select COUNT(*) from pg_tables WHERE tablename='";
        sql += _tableName;
        sql += "';";

        PGresult* res = PQexec((PGconn*)m_conn, sql);
        if (!res)
        {
            RM_LOG_ERROR("Fail to execute " << sql << ": " << PQerrorMessage((PGconn*)m_conn));
            return -1;
        }

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            RM_LOG_ERROR("Fail to execute: " << PQresultErrorMessage(res));
            PQclear(res);
            return -1;
        }

        char* value = PQgetvalue(res, 0, 0);
        int z = 0;
        if (0 == strcmp(value, "1"))
            z = 1;

        PQclear(res);
        return z;
    }

    int CPostgresSQL::createTable(const tchar* _tableName, int _count, const tchar** _colNames, const tchar** _colType, const int* _colSize)
    {
        if (!_tableName || !_colNames || !_colType || !_colSize)    return -1;
        CStdString sql = "CREATE TABLE ";
        sql += _tableName;
        sql += "(";

        for (int i = 0; i < _count; ++i)
        {
            CStdString v;
            if (_colSize[i] != 0)
                v.Format("%d", _colSize[i]);

            sql += _colNames[i];
            sql += " ";
            sql += _colType[i];

            if (_colSize[i] != 0)
            {
                sql += " (";
                sql += v;
                sql += ") ";
            }

            if (i != _count - 1) sql += ", ";
        }

        sql += ")";
        PGresult* res = PQexec((PGconn*)m_conn, sql);

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            RM_LOG_ERROR("Fail to execute: " << PQresultErrorMessage(res));
            PQclear(res);
            return -1;
        }

        return 0;
    }

    int CPostgresSQL::exec(const tchar* _sql)
    {
        if (!_sql)  return -1;

        PGresult* res = PQexec((PGconn*)m_conn, _sql);
        if (!res)
        {
            RM_LOG_ERROR("Fail to execute: " << _sql);
            return -1;
        }

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            RM_LOG_ERROR("Fail to execute: " << PQresultErrorMessage(res));
            PQclear(res);
            return -1;
        }

        char* value = PQcmdTuples(res);

        return atoi_t(value);
    }

    int CPostgresSQL::beginCopy(const tchar* _sql)
    {
        if (!_sql)  return -1;

        m_res = PQexec((PGconn*)m_conn, _sql);
        if (PGRES_COPY_IN == PQresultStatus((PGresult*)m_res))
        {
            return 0;
        }

        return -1;
    }

    int CPostgresSQL::copy(const char* _data, int _len)
    {
        if (!_data) return -1;

    retry:
        int z = PQputCopyData((PGconn*)m_conn, _data, _len);
        if (1 == z) return 0;
        else if (0 == z)    goto retry;
        else
        {
            RM_LOG_ERROR(PQerrorMessage((PGconn*)m_conn));
        }

        return -1;
    }

    int CPostgresSQL::endCopy()
    {
        if (PQputCopyEnd((PGconn*)m_conn, NULL) == 1)
        {
            PQclear((PGresult*)m_res);

            m_res = PQgetResult((PGconn*)m_conn);
            if (PGRES_COMMAND_OK == PQresultStatus((PGresult*)m_res))
            {
                char* value = PQcmdTuples((PGresult*)m_res);
                return atoi_t(value);
            }
            else
                puts(PQerrorMessage((PGconn*)m_conn));
        }

        return -1;
    }
}
