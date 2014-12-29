#include "capture.h"
#include "global.h"
#include "file_system.h"
#include "string_.h"
#include "packet_ring.h"
#include "net_util.h"
#include "file.h"

vector<DeviceST> CNetCapture::m_localNICs;

CNetCapture::CNetCapture()
{
    m_devIndex          = -1;
    m_nextFileIndex     = NULL; //g_env->m_config.engine.startFileIndex;// +1;
    m_curTarget         = -1;
    m_curDB             = 0;
    bzero(&m_pktRecvCount, sizeof(m_pktRecvCount));
    bzero(&m_byteRecvCount, sizeof(m_byteRecvCount));
    m_tickCount         = 0;
    m_fileInfo          = NULL;
    m_pcapFile          = NULL;

    for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
    {
        m_captureState[i] = CAPTURE_STOPPED;
    }

    m_createFile = false;
}

CNetCapture::~CNetCapture()
{
    if (m_createFile)
    {
        SAFE_DELETE(m_pcapFile);
        SAFE_DELETE(m_fileInfo);
    }
}

bool CNetCapture::init(bool _createFile)
{
    if (0 != createTables())
        return false;

    if (_createFile)
    {
        if (!m_fileInfo)
        {
            m_fileInfo = &g_env->m_config.storage.fileDir[m_curTarget].fileInfo;
        }

        if (!m_pcapFile)
            m_pcapFile = createCaptureFile();

        return m_pcapFile != NULL;
    }

    m_createFile = _createFile;
    return true;
}

/**
 * Only show the NICs which has valid IP address
 **/
int CNetCapture::showLocalNICs()
{
    if (m_localNICs.size() == 0)
    {
        printf("No NIC available.\n");
        return 0;
    }

    for (unsigned int i = 0; i < m_localNICs.size(); ++i)
    {
        DeviceST& device = m_localNICs[i];
        CStdString strPorts;
        strPorts.Format("%d Mb", /*device.portIndex, device.portIndex + device.portCount - 1, */device.linkSpeed * device.portCount);

        printf("%2d: %s %s\n", i+1, device.desc.c_str(), strPorts.c_str());
    }

    return m_localNICs.size();
}

int CNetCapture::getLocalNICs(vector<DeviceST>& _NICs)
{
    for (unsigned int i = 0; i < m_localNICs.size(); ++i)
    {
        _NICs.push_back(m_localNICs[i]);
    }

    return _NICs.size();
}

int CNetCapture::openDevice(int _index)
{
    if (_index >= (int)m_localNICs.size())   return -1;
    if (_index == m_devIndex)    return 0;

    m_devIndex = _index;
    m_device = m_localNICs[_index];
    if (openDevice(_index, m_device.name))
    {
        if (g_env->m_config.engine.debugMode >= DEBUG_BLOCK)
        {
            u_int32 hashValue = HashString(m_device.name);
            CStdString  logName;
            logName.Format("%sPerfTest/CaptureBlock_%u.txt", LiangZhu::GetAppDir(), hashValue);
            if (!m_fsCaptureBlock.is_open())
                m_fsCaptureBlock.open(logName, ios_base::out | ios_base::trunc);

            if (g_env->m_config.engine.debugMode >= DEBUG_PACKET)
            {
                for (int i = 0; i < MAX_CAPTURE_THREAD; ++i)
                {
                    logName.Format("%sPerfTest/CapturePacket_%u_%u.txt", GetAppDir(), hashValue, i);
                    if (!m_fsCapturePacket[i].is_open())
                        m_fsCapturePacket[i].open(logName, ios_base::out | ios_base::trunc);
                }
            }
        }

        RM_LOG_INFO("Success to open [" << _index << "]: " << m_device.name << ", " << m_device.desc);
        for (unsigned int i = 0; i < m_device.portCount; ++i)
        {
            CStdString status;
            if (m_device.portStat[i].status == LINK_ON)           status = "UP";
            else if (m_device.portStat[i].status == LINK_OFF)     status = "DOWN";
            else if (m_device.portStat[i].status == LINK_ERROR)   status = "ERROR";
            else if (m_device.portStat[i].status == LINK_UNKNOWN) status = "UNKNOWN";

            RM_LOG_INFO("Port" << i << " is " << status);
        }
        return 0;
        //return createStatsTable();
    }

    m_devIndex = -1;
    return -1;
}

int CNetCapture::setCaptureFilter(const char* _filterString, int _port)
{
    Filter_t filter;
    if (0 != ParseBPFilter(filter, _filterString, _port))
    {
        RM_LOG_ERROR("Fail to parse the filter: " << _filterString);
        return -1;
    }

    RM_LOG_INFO("Set capture filter: " << _filterString);

    return setCaptureFilter_(filter, _port, _filterString);
}

CStdString CNetCapture::getFilterString(const Filter_t& _filterItem)
{
    CStdString strFilter;

    return strFilter;
}

int CNetCapture::createDB(int _index, CStdString& _dbPath)
{
    assert(_index < 10);
    int z = 0;

    if (0 != m_dbs[_index].open(_dbPath))/* ||
        0 != m_dbs[_index].exec("PRAGMA journal_mode = TRUNCATE", this, NULL))*/
    {
        RM_LOG_ERROR("Fail to open database: " << _dbPath);
        return -1;
    }

    if (!m_dbs[_index].isTableExist(FILE_STATUS_TABLE))
    {
        const char* sql = "CREATE TABLE "   \
            FILE_STATUS_TABLE "("  \
            "NAME INT PRIMARY KEY NOT NULL," \
            "AGE                INT NOT NULL," \
            "FIRST_PACKET_TIME  NUMERIC NOT NULL," \
            "LAST_PACKET_TIME   NUMERIC NOT NULL," \
            "STATUS             INT," \
            "SIZE               NUMERIC NOT NULL," \
            "PACKET_COUNT       NUMERIC NOT NULL," \
            "BYTES_COUNT        NUMERIC NOT NULL);";

        z = m_dbs[_index].exec(sql, this, NULL);
        if (0 == z)
        {
            sql = "CREATE INDEX index_FPT ON " FILE_STATUS_TABLE " (FIRST_PACKET_TIME)";
            m_dbs[_index].exec(sql, this, NULL);

            sql = "CREATE INDEX index_LPT ON " FILE_STATUS_TABLE " (LAST_PACKET_TIME)";
            m_dbs[_index].exec(sql, this, NULL);
        }
        else
        {
            RM_LOG_ERROR("Fail to execute: " << sql);
        }
    }

    return z;
}

int CNetCapture::createTables()
{
    int z = 0;
    CStdString dbPath;

    for (int i = 0; i < GetRecordDBCount(); ++i)
    {
        CStdString targetName;
        targetName.Format("target%d", i);
        dbPath.Format("%s/%s/" FILE_DB_NAME, g_env->m_config.engine.dbPath, targetName.c_str());
        z = createDB(i, dbPath);
    }

    if (0 != z) return z;
    return createExtTables();
}

#define BEGIN_SQL(sql)          //sql += "'";
#define ADD_COL_SQL(sql, col)      sql += "'"; sql += col; sql += "', ";
#define ADD_LAST_COL_SQL(sql, col) sql += "'"; sql += col; sql += "');";


/**
 * The pcap file name will be like index-startTime-endTime-age
 **/
bool CNetCapture::updateFileName()
{
    CStdString oldName = m_pcapFile->getFileName();
    if (oldName.empty())    return false;

    //
    // check whether lock the file
    //
    for (unsigned int i = 0; i < g_env->m_config.capture.lockTime.size(); ++i)
    {
        if (m_fileInfo->lastPacketTime >= g_env->m_config.capture.lockTime[i].startTime &&
            m_fileInfo->firstPacketTime <= g_env->m_config.capture.lockTime[i].stopTime)
        {
            m_fileInfo->status = STATUS_LOCKED;
            break;
        }
    }

    int fileIndex = m_fileInfo->index;
    AdjustFileIndex(m_curTarget, fileIndex);

#if defined(DEBUG) || defined(_DEBUG)
    vector<CStdString> values;
    StrSplit(oldName, "-", values);

    CStdString subDir = GetFileSubDirByIndex(fileIndex, g_env->m_config.storage.dirLevel);
    CStdString strIndex;
    strIndex.Format("%04d", fileIndex % g_filesPerDir);
    subDir += strIndex;

    assert(values[0] == g_env->m_config.storage.fileDir[m_curTarget].target + "/" + subDir);
    if (g_env->m_config.storage.renameFile)
        assert(atoi(values[4].c_str()) == m_fileInfo->age || atoi(values[4].c_str()) + 1 == m_fileInfo->age);
#endif

    m_fileInfo->size  = m_pcapFile->getOffset();
    bool hasRetried = false;

retry:
    bool ret = true;

    if (g_env->m_config.storage.renameFile)
    {
        CStdString newName;
        newName.Format("%s/%s%04d-" I64D "-" I64D "-%d-%d-" I64D ".pcap",
                       g_env->m_config.storage.fileDir[m_curTarget].target.c_str(),
                       GetFileSubDirByIndex(fileIndex, g_env->m_config.storage.dirLevel).c_str(),
                       fileIndex % g_filesPerDir,
                       m_fileInfo->firstPacketTime,
                       m_fileInfo->lastPacketTime,
                       m_fileInfo->status,
                       m_fileInfo->age,
                       m_fileInfo->size);

        RM_LOG_DEBUG("Update file name = " << newName);

        ret = m_pcapFile->rename(newName) == 0;
        //
        // may be in the situation that the newName file is existed.
        // so, remove the exist one and retry
        //
        if (!ret && !hasRetried)
        {
            RM_LOG_INFO("Try to remove the existing file.");
            DeletePath(newName);
            hasRetried = true;
            goto retry;
        }
        else if (!ret && hasRetried)
        {
            RM_LOG_ERROR("Fail to rename file from '" << oldName << "' to '" << newName << "'");
        }
    }

    //
    // save the file information into database
    //
    if (ret)
    {
        CStdString targetPath = g_env->m_config.storage.fileDir[m_curTarget].target.c_str();
        CStdString sql;
        sql.Format("REPLACE INTO " FILE_STATUS_TABLE " VALUES(%d, %d, " I64D ", " I64D ", %d, " I64D ", " I64D ", " I64D ")",
            m_fileInfo->index,
            m_fileInfo->age,
            m_fileInfo->firstPacketTime,
            m_fileInfo->lastPacketTime,
            m_fileInfo->status,
            m_fileInfo->size,
            m_fileInfo->packetCount,
            m_fileInfo->bytesCount);

        int z = 0;
        do
        {
            z = m_dbs[m_curDB].exec(sql, this, NULL);
            if (0 == z) break;
            else if (SQLITE_BUSY != z)    // database is locked
            {
                RM_LOG_ERROR("Fail to execute: " << sql);
                break;
            }
            
        } while (g_env->enable());
        
        ret = (0 == z);
        if (!ret && g_env->m_config.storage.renameFile)  // if fail to store the new filename in the db, should be return to the old file name
        {
            RM_LOG_ERROR("Fail to record new file name to " << m_fileInfo->index << ", path: " << targetPath);
            m_pcapFile->rename(oldName);
        }
    }

    return ret;
}

int CNetCapture::GetRecord_Callback(void* _context, int _argc, char** _argv, char** _szColName)
{
    CNetCapture* captureObj = (CNetCapture*)_context;
    if (captureObj)
    {
        for (int i = 0; i < _argc; ++i)
        {
            if (0 == stricmp(_szColName[i], "NAME"))
            {
                captureObj->m_fileInfo->index = atoi(_argv[i]);
                assert(*captureObj->m_nextFileIndex == captureObj->m_fileInfo->index);
            }
            else if (0 == stricmp(_szColName[i], "AGE"))
                captureObj->m_fileInfo->age = atoi(_argv[i]);
            else if (0 == stricmp(_szColName[i], "FIRST_PACKET_TIME"))
                captureObj->m_fileInfo->firstPacketTime= atoi64(_argv[i]);
            else if (0 == stricmp(_szColName[i], "LAST_PACKET_TIME"))
                captureObj->m_fileInfo->lastPacketTime = atoi64(_argv[i]);
            else if (0 == stricmp(_szColName[i], "STATUS"))
                captureObj->m_fileInfo->status = atoi(_argv[i]);
            else if (0 == stricmp(_szColName[i], "SIZE"))
                captureObj->m_fileInfo->size = atoi64(_argv[i]);
        }
    }

    return 0;
}

CStdString CNetCapture::getNextFileName()
{
    bzero(m_fileInfo, sizeof(FileInfo_t));

    //
    // m_nextFileIndex is for all the directory, but when to get the file path
    // it has to be local
    //
    int fileIndex = *m_nextFileIndex;
    AdjustFileIndex(m_curTarget, fileIndex);

    int z = 0;
    CStdString sql;
    sql.Format("SELECT * FROM " FILE_STATUS_TABLE " WHERE name=%d", *m_nextFileIndex);

    do
    {
        int z = m_dbs[m_curDB].exec(sql, this, GetRecord_Callback);
        if (z == 0) break;
        else if (z != SQLITE_BUSY)
        {
            RM_LOG_ERROR("Fail to execute: " << sql);
            return "";
        }
    } while (g_env->enable());

    if (0 != z) return "";

    CStdString fileName;
    if (g_env->m_config.storage.renameFile)   // don't update
    {
        fileName.Format("%04d-" I64D "-" I64D "-%d-%d-" I64D ".pcap",
            fileIndex % g_filesPerDir,    // there may no record, so use m_nextFileIndex rather than m_fileInfo->index
            m_fileInfo->firstPacketTime,
            m_fileInfo->lastPacketTime,
            m_fileInfo->status,
            m_fileInfo->age,
            m_fileInfo->size);
    }
    else
    {
        fileName.Format("%04d-0-0-0-0-0.pcap",  fileIndex % g_filesPerDir);
    }
    m_fileInfo->index = *m_nextFileIndex;
    m_fileInfo->firstPacketTime = m_fileInfo->lastPacketTime = 0;
    m_fileInfo->size = 0;
    m_fileInfo->packetCount = m_fileInfo->bytesCount = 0;

    return GetFileSubDirByIndex(fileIndex, g_env->m_config.storage.dirLevel) + fileName;
}

CStdString CNetCapture::getFilePath(u_int32 _index)
{
    bzero(&m_fileInfo, sizeof(m_fileInfo));
    CStdString filePath, fileName;

    //
    // find the file location
    //
    unsigned int i = 0, count = 0;
    for (; i < g_env->m_config.storage.fileDir.size(); ++i)
    {
        count += g_env->m_config.storage.fileDir[i].capacity / g_env->m_config.storage.fileSize;
        if (_index <= count - 1)
        {
            filePath = g_env->m_config.storage.fileDir[i].target;
            break;
        }
    }
    int index = _index;
    AdjustFileIndex(i, index);

    CStdStringA sql;
    sql.Format("SELECT * FROM " FILE_STATUS_TABLE " WHERE name=%d", _index);
    int z = m_dbs[m_curDB].exec(sql, this, GetRecord_Callback);
    if (z != 0)
    {
        RM_LOG_ERROR("Fail to execute: " << sql);
        return "";
    }

    fileName.Format("%04d-" I64D "-" I64D "-%d-%d-" I64D ".pcap",
        _index % g_filesPerDir,
        m_fileInfo->firstPacketTime,
        m_fileInfo->lastPacketTime,
        m_fileInfo->status,
        m_fileInfo->age,
        m_fileInfo->size);

    return filePath;
}

int CNetCapture::nextFile(bool _next)
{
    //
    // find the next file location
    //
    int retry = 0;

nextfile:
    int count = 0;
    if (g_env->m_config.storage.writeMode == WRITE_INTERLEAVED)
    {
        count = g_env->m_config.storage.fileDir[m_curTarget].fileCount;
        if (*m_nextFileIndex >= count)
        {
            *m_nextFileIndex = 0;
        }
    }
    else
    {
        unsigned int i = 0;
        count = 0;
        for (; i < g_env->m_config.storage.fileDir.size(); ++i)
        {
            count += g_env->m_config.storage.fileDir[i].fileCount;
            if (*m_nextFileIndex < count)
            {
                m_curTarget = i;
                break;
            }
        }

        if (i == g_env->m_config.storage.fileDir.size())    // all the capacity is used up, so round up
        {
            m_curTarget = 0;
            *m_nextFileIndex = 0;
        }
    }

    CStdString fileName = g_env->m_config.storage.fileDir[m_curTarget].target;
    fileName += "/";
    fileName += getNextFileName();  // use the index as key to get the full file name from db
    if (m_fileInfo->status != STATUS_NORMAL &&
        m_fileInfo->status != STATUS_CAPTURE)
    {
        RM_LOG_INFO("File is not available: " << fileName);
        (*m_nextFileIndex)++;  // move to the next file index

        goto nextfile;
    }

    RM_LOG_DEBUG("Next file = " << fileName);
    int z = 0;
    if (0 == m_pcapFile->open(fileName, ACCESS_WRITE, FILE_OPEN_ALWAYS, true, g_env->m_config.storage.secAlign))
    {
        m_fileInfo->status = STATUS_CAPTURE;
        if (updateFileName())   // restore the file name to n-0-0-0-0 before to write
        {
            if (!_next)
            {
                if (m_fileInfo->size > 0)
                    m_pcapFile->setFilePos(m_fileInfo->size);    // set the position at last written to
            }
            else
            {
                (*m_nextFileIndex)++;  // move to the next file index
            }
        }
        else
        {
            RM_LOG_ERROR("Fail to clear the file name: " << fileName);
            z = -1;
        }
    }
    else
    {
        RM_LOG_ERROR("Fail to open the file: " << fileName);
        z = -1;
    }

    return z;
}

u_int64 CNetCapture::getCapability()
{
    return m_capability;
}


int CNetCapture::padding(DataBlock_t* _block, int _padSize)
{
    while (_padSize > 0)
    {
        packet_header_t fakeHeader;
        bzero(&fakeHeader, sizeof(packet_header_t));
        byte pad[ONE_KB * 64] = { 0 };
        int len = MyMin((size_t)65535, (size_t)(_padSize));
        _padSize -= len;
        if (_padSize > 0 && _padSize <= (int)sizeof(packet_header_t))
        {
            len -= sizeof(packet_header_t);
            _padSize += sizeof(packet_header_t);
        }

        fakeHeader.len = fakeHeader.caplen = len - sizeof(packet_header_t);

        byte* dst = _block->data + _block->dataDesc.usedSize;
        memcpy(dst, &fakeHeader, sizeof(packet_header_t));
        dst += sizeof(packet_header_t);
        memcpy(dst, pad, fakeHeader.caplen);
        _block->dataDesc.usedSize += len;
    }

    return 0;
}

int ParseBPFilter(Filter_t& _filter, const char* _filterString, int _port)
{
    vector<CStdString> keys;
    StrSplit(_filterString, " ", keys);

    bzero(&_filter, sizeof(Filter_t));
    _filter.segment = -1;
    bool isOK = true;

    unsigned int i = 0;
    bool src = false, dst = false;
    for (i = 0; i < keys.size(); ++i)
    {
        if (!isOK)  break;

        CStdString key = keys[i];
        key.ToLower();

        if (key == "not")
            _filter.isDrop = true;
        else if (key == "symm")
            _filter.isSymm = true;
        else if (key == "udp")
            _filter.isUDP = true;
        else if (key == "tcp")
            _filter.isTCP = true;
        else if (key == "src")
        {
            if (src || dst) { isOK = false; break; }
            src = true;
        }
        else if (key == "dst")
        {
            if (src || dst) { isOK = false; break; }
            dst = true;
        }
        else if (key == "net")
        {
            if (i + 1 == keys.size()) { isOK = false;   break; }

            vector<CStdString> values;
            StrSplit(keys[i + 1], "/", values);
            if (values.size() != 2) { isOK = false; break; }

            u_int32 addr = ntohl(inet_addr(values[0]));
            if (addr == (u_int32)-1) { isOK = false; break; }

            if (src)
            {
                src = false;
                _filter.isSrcNet = true;
                _filter.srcNet = addr;
                _filter.srcNetSize = atoi(values[1]);
            }
            else if (dst)
            {
                dst = false;
                _filter.isDstNet = true;
                _filter.dstNet = addr;
                _filter.dstNetSize = atoi(values[1]);
            }
            else
            {
                _filter.srcNet = addr;
                _filter.srcNetSize = atoi(values[1]);
            }
            ++i;
        }
        else if (key == "host")
        {
            if (i + 1 == keys.size()) { isOK = false;   break; }

            u_int32 addr = ntohl(inet_addr(keys[i + 1]));
            if (addr == (u_int32)-1)
            {
                isOK = false;
                break;
            }

            if (src)
            {
                src = false;
                _filter.isSrcHost = true;
                _filter.srcHost = addr;
            }
            else if (dst)
            {
                dst = false;
                _filter.isDstHost = true;
                _filter.dstHost = addr;
            }
            else
            {
                _filter.srcHost = addr;
            }
            ++i;
        }
        else if (key == "port")
        {
            if (i + 1 == keys.size()) { isOK = false;   break; }

            vector<CStdString> ports;
            StrSplit(keys[i + 1], "-", ports);
            if (ports.size() == 1)
            {
                if (src)
                {
                    src = false;
                    _filter.isSrcPort = true;
                    _filter.srcPortStart = _filter.srcPortStop = atoi(ports[0]);
                }
                else if (dst)
                {
                    dst = false;
                    _filter.isDstPort = true;
                    _filter.dstPortStart = _filter.dstPortStop = atoi(ports[0]);
                }
                else
                {
                    _filter.srcPortStart = _filter.srcPortStop = atoi(ports[0]);
                }
            }
            else if (ports.size() == 2)
            {
                if (src)
                {
                    src = false;
                    _filter.isSrcPort = true;
                    _filter.srcPortStart = atoi(ports[0]);
                    _filter.srcPortStop = atoi(ports[1]);
                }
                else if (dst)
                {
                    dst = false;
                    _filter.isDstPort = true;
                    _filter.dstPortStart = atoi(ports[0]);
                    _filter.dstPortStop = atoi(ports[1]);
                }
            }
            else
            {
                isOK = false;
                break;
            }
            ++i;
        }
        else if (key == "segment")
        {
            _filter.segment = atoi(keys[i + 1]);
            ++i;
        }
        else if (key == "slice")
        {
            _filter.slice = atoi(keys[i + 1]);
            ++i;
        }
        else if (key == "mac")
        {
            if (i + 1 == keys.size()) { isOK = false;   break; }

            u_int64 macAddr = ChangQian::GetMacAddr(keys[i + 1]);
            if (macAddr == (u_int32)-1)
            {
                isOK = false;
                break;
            }

            if (src)
            {
                _filter.isSrcMac = true;
                _filter.srcMac = macAddr;
                src = false;
            }
            else if (dst)
            {
                _filter.isDstMac = true;
                _filter.dstMac = macAddr;
                dst = false;
            }
            else
            {
                _filter.srcMac = macAddr;
            }
            ++i;
        }
    }

    assert(!(_filter.isTCP && _filter.isUDP));
    assert(_filter.dstHost == 0 || (_filter.dstHost != 0 && _filter.isDstHost));

    if (i != keys.size())   return -1;
    _filter.valid = i > 0;
    return 0;
}

CStdString GenQuerySql(Filter_t _bpfFilter)
{
    CStdString sql;
    Pred_e  opt = PredNone;

    if (_bpfFilter.isTCP || _bpfFilter.isUDP)
    {
        sql += "PROTOCOL = ";
        CStdString proValue;
        proValue.Format("%u", _bpfFilter.isTCP ? 6 : 17);
        sql += proValue;

        opt = PredAnd;
    }

    if (_bpfFilter.segment != (u_int16)-1)
    {
        if (opt == PredAnd) sql += " AND ";
        else if (opt == PredOr) sql += " OR ";

        sql += "PORT_NUM = ";
        CStdString segValue;
        segValue.Format("%u", _bpfFilter.segment);
        sql += segValue;

        opt = PredAnd;
    }

    if (_bpfFilter.srcHost == 0 && _bpfFilter.srcPortStart == 0 &&
        _bpfFilter.dstHost == 0 && _bpfFilter.dstPortStart == 0)
        return sql;

    if (opt == PredAnd) sql += " AND ";
    else if (opt == PredOr) sql += " OR ";

    sql += "(";

again:
    opt = PredNone;
    sql += "(";

    if (_bpfFilter.srcHost != 0)
    {
        sql += "SRC_IP = ";
        CStdString addr;
        addr.Format("%u", _bpfFilter.srcHost);
        sql += addr;

        opt = PredAnd;
    }

    if (_bpfFilter.srcPortStart != 0)
    {
        if (opt == PredAnd) sql += " AND ";
        else if (opt == PredOr) sql += " OR ";

        sql += "SRC_PORT = ";
        CStdString portValue;
        portValue.Format("%u", _bpfFilter.srcPortStart);
        sql += portValue;

        opt = PredAnd;
    }

    if (_bpfFilter.dstHost != 0)
    {
        if (opt == PredAnd) sql += " AND ";
        else if (opt == PredOr) sql += " OR ";

        sql += "DST_IP = ";
    /*    in_addr in;
        in.s_addr = _bpfFilter.dstHost;
        sql += inet_ntoa(in);
*/
        CStdString addr;
        addr.Format("%u", _bpfFilter.dstHost);
        sql += addr;

        opt = PredAnd;
    }

    if (_bpfFilter.dstPortStart != 0)
    {
        if (opt == PredAnd) sql += " AND ";
        else if (opt == PredOr) sql += " OR ";

        sql += "DST_PORT = ";
        CStdString portValue;
        portValue.Format("%u", _bpfFilter.dstPortStart);
        sql += portValue;

        opt = PredAnd;
    }

    sql += ")";

    if (_bpfFilter.isSymm)
    {
        sql += " OR ";
        Swap(_bpfFilter.srcHost, _bpfFilter.dstHost);
        Swap(_bpfFilter.isSrcHost, _bpfFilter.isDstHost);
        Swap(_bpfFilter.srcPortStart, _bpfFilter.dstPortStart);
        Swap(_bpfFilter.srcPortStop, _bpfFilter.dstPortStop);
        Swap(_bpfFilter.isSrcPort, _bpfFilter.isDstPort);

        _bpfFilter.isSymm = false;
        goto again;
    }

    sql += ")";

    return sql;
}
