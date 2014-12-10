#include "file.h"
#include "file_system.h"
#include "file_pool.h"
#include "gtest/gtest.h"

using namespace LiangZhu;


/**
 * @Function: Use CHFile to do synchronous operation
 **/
class CHFileTestSync : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }

    CHFileTestSync()
    {
        m_fileName = "./CHFileTestSync.txt";
        DeletePath(m_fileName);     // make sure no test file exist

        //
        // fill the buffer with random values
        //
        srand(time(NULL));
        for (int i = 0; i < ONE_MB; ++i)
        {
            m_buffer[i] = rand();
        }
    }

    virtual ~CHFileTestSync()
    {
        DeletePath(m_fileName);     // make sure no test file exist
    }

    /**
     * @Function: Prepare the file and open it with sync flag
     **/
    virtual void SetUp()
    {
        ASSERT_FALSE(IsFileExist(m_fileName));  // file shouldn't exist

        m_file.setFileName(m_fileName);
        m_file.open(ACCESS_READ | ACCESS_WRITE, FILE_OPEN_ALWAYS, false, false);
        ASSERT_TRUE(m_file.isValid());

        ASSERT_TRUE(IsFileExist(m_fileName));   // file should been created
    }

    /**
     * @Function: Cleanup the testing file
     **/
    virtual void TearDown()
    {
        m_file.close();
        ASSERT_FALSE(m_file.isValid());

        DeletePath(m_fileName);
        ASSERT_FALSE(IsFileExist(m_fileName));  // file shouldn't exist
    }

    virtual void TestWriteAndRead(u_int64 _offset);

protected:
    CHFile          m_file;
    CStdString      m_fileName;
    byte            m_buffer[ONE_MB];
};

void CHFileTestSync::TestWriteAndRead(u_int64 _offset)
{
    int bytesWritten = m_file.write_w(m_buffer, ONE_MB, _offset);
    ASSERT_TRUE(bytesWritten == ONE_MB);

    AutoFree<byte, _RELEASE_BYTE_> buf(new byte[ONE_MB], DeleteArray);
    m_file.seek(_offset);
    int bytesRead = m_file.read_w(buf, ONE_MB, _offset);
    ASSERT_TRUE(bytesRead == ONE_MB);

    ASSERT_TRUE(memcmp(m_buffer, buf, ONE_MB) == 0);
    ASSERT_TRUE(m_file.getPos() == ONE_MB + _offset);
}

/**
* @Function: Test write and read data in synchronous way
*  1. Write ONE_MB data at offset = 0.
*  2. Read the written data at offset = 0.
*  3. Compare the data.
**/
TEST_F(CHFileTestSync, WriteAndRead)
{
    TestWriteAndRead(0);        // test write and read at offset = 0
    TestWriteAndRead(1134);     // test write and read at offset = 1134
    TestWriteAndRead(1134);     // test write and read at offset = 1134
}


/*
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ################################################################
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

/**
 * @Function: Use CHFile to do asynchronous operation
 **/
class CHFileTestAsync : public CHFileTestSync
{
public:
    CHFileTestAsync() : m_filePool(1024, 0)
    {
        m_filePool.setAioCallback(AioCallback);
        m_ioFinished = false;
    }
    
    /**
     * @Function: Prepare the file and open it with async flag
     **/
    virtual void SetUp()
    {
        ASSERT_FALSE(IsFileExist(m_fileName));  // file shouldn't exist

        m_file.setFilePool(&m_filePool);
        m_file.setFileName(m_fileName);
        m_file.open(ACCESS_READ | ACCESS_WRITE, FILE_OPEN_ALWAYS, true, false);
        ASSERT_TRUE(m_file.isValid());

        int z = m_filePool.attach(m_file);  
        ASSERT_TRUE(z == 0);

        ASSERT_TRUE(IsFileExist(m_fileName));   // file should been created
    }

    virtual void TestWriteAndRead(u_int64 _offset);

protected:
    static int AioCallback(void* _handler, PPER_FILEIO_INFO_t _request)
    {
        CHFileTestAsync* pThis = (CHFileTestAsync*)_request->context;
        pThis->m_ioFinished = true;

        delete _request;
        return 0;
    }

protected:
    CFilePool       m_filePool;
    bool            m_ioFinished;
};

void CHFileTestAsync::TestWriteAndRead(u_int64 _offset)
{
#ifdef WIN32
    RM_LOG_INFO("The following error is intentionally");
#endif

    int bytesWritten = m_file.write_w(m_buffer, ONE_MB, 0); // not workable

#ifdef WIN32
    ASSERT_TRUE(bytesWritten == 0);
#else
    ASSERT_TRUE(bytesWritten == ONE_MB);
#endif

    PPER_FILEIO_INFO_t   request = new PER_FILEIO_INFO_t;
    bzero(request, sizeof(OVERLAPPED));
    request->buffer = m_buffer;
    request->dataLen = ONE_MB;
    request->Offset = (u_int32)_offset;
    request->context = this;

    int z = m_file.write(request);
    ASSERT_TRUE(z == 0);
    int waitCount = 0;
    while (!m_ioFinished)
    {
        SleepMS(100);
        m_filePool.getFinishedRequest();    // get the finished write request
        if (++waitCount >= 10)
            FAIL();
    }
    m_ioFinished = false;

    //
    // read the written ONE_MB block data
    //
    AutoFree<byte, _RELEASE_BYTE_> buf(new byte[ONE_MB], DeleteArray);
    request = new PER_FILEIO_INFO_t;
    bzero(request, sizeof(OVERLAPPED));
    request->buffer = buf;
    request->dataLen = ONE_MB;
    request->Offset = (u_int32)_offset;
    request->context = this;

    z = m_file.read(request);
    ASSERT_TRUE(z == 0);
    waitCount = 0;
    while (!m_ioFinished)
    {
        SleepMS(100);
        m_filePool.getFinishedRequest();    // get the finished write request
        if (++waitCount >= 10)
            FAIL();
    }
    m_ioFinished = false;

    ASSERT_TRUE(memcmp(m_buffer, buf, ONE_MB) == 0);
}

/**
 * @Function: Test write and read data in asynchronous way
 **/
TEST_F(CHFileTestAsync, WriteAndRead)
{
    TestWriteAndRead(0);        // test write and read at offset = 0
    TestWriteAndRead(1134);     // test write and read at offset = 1134
    TestWriteAndRead(1134);     // test write and read at offset = 1134
}


/*
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
################################################################
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

/**
* @Function: Use CHFile to do synchronous direct io operation
**/
class CHFileTestDirectSync : public CHFileTestSync
{
public:
    CHFileTestDirectSync()
    {
        m_buffer = (byte*)AlignedAlloc(ONE_MB, SECTOR_ALIGNMENT);
    }

    ~CHFileTestDirectSync()
    {
        if (m_buffer)
            AlignedFree(m_buffer);
    }

    /**
    * @Function: Prepare the file and open it with async flag
    **/
    virtual void SetUp()
    {
        ASSERT_FALSE(IsFileExist(m_fileName));  // file shouldn't exist

        m_file.setFileName(m_fileName);
        m_file.open(ACCESS_READ | ACCESS_WRITE, FILE_OPEN_ALWAYS, false, true);
        ASSERT_TRUE(m_file.isValid());

        ASSERT_TRUE(IsFileExist(m_fileName));   // file should been created
    }

protected:
    virtual void TestWriteAndRead(u_int64 _offset);
    
private:
    byte*   m_buffer;
};

void CHFileTestDirectSync::TestWriteAndRead(u_int64 _offset)
{
    int bytesWritten = m_file.write_w(m_buffer, ONE_MB, _offset);
    if (_offset % SECTOR_ALIGNMENT == 0 &&
        (u_int64)m_buffer % SECTOR_ALIGNMENT == 0)
        ASSERT_TRUE(bytesWritten == ONE_MB);
    else
        ASSERT_TRUE(bytesWritten == 0);

    AutoFree<byte, _RELEASE_VOID_> buf((byte*)AlignedAlloc(ONE_MB, SECTOR_ALIGNMENT), AlignedFreeFunc);

    int bytesRead = m_file.read_w(buf, ONE_MB, _offset);
    if (_offset % SECTOR_ALIGNMENT == 0)
    {
        ASSERT_TRUE(bytesRead == ONE_MB);
        ASSERT_TRUE(memcmp(m_buffer, buf, ONE_MB) == 0);
        ASSERT_TRUE(m_file.getPos() == ONE_MB + _offset);
    }
    else
    {
        ASSERT_TRUE(bytesRead == 0);
    }
}


/**
* @Function: Test write and read data in synchronous and direct io way
**/
TEST_F(CHFileTestDirectSync, WriteAndRead)
{
    TestWriteAndRead(0);        // test write and read at offset = 0
    TestWriteAndRead(1134);     // test write and read at offset = 1134
    TestWriteAndRead(4096);     // test write and read at offset = 4096
    TestWriteAndRead(4567);     // test write and read at offset = 4567
}