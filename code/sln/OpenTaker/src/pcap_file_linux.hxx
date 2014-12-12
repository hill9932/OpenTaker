#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

int CPCAPFile::setFileSize(u_int64 _fileSize)
{
    if (INVALID_HANDLE_VALUE == m_fileHandle) return -1;

    int z = truncate(m_fileName, _fileSize);

    m_fileSize = _fileSize;
    return z;
}

int CPCAPFile::createMapFile(const tchar* _fileName, u_int64 _fileSize)
{
    m_fileHandle = ::open(_fileName,
                     _fileSize > 0 ? O_CREAT | O_RDWR : O_RDONLY, 
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); 

    ON_ERROR_LOG_LAST_ERROR_AND_DO(m_fileHandle, == , INVALID_HANDLE_VALUE, return err);

    u_int32 low32Size = 0;
    if (_fileSize != 0 && _fileSize != m_fileSize) setFileSize(_fileSize);
    else
    {
        m_fileSize = 0;
        low32Size = getSize((u_int32*)&m_fileSize);
        m_fileSize <<= 32;
        m_fileSize += low32Size;
    }


    //lseek64(m_hFile, _fileSize, SEEK_SET);     

    m_buf = (byte*)mmap(NULL, 
                        m_fileSize,
                        _fileSize > 0 ? PROT_READ | PROT_WRITE : PROT_READ,
                        MAP_SHARED, 
                        m_fileHandle, 
                        0);

    ON_ERROR_LOG_LAST_ERROR_AND_DO(m_buf, == , MAP_FAILED, return err);

    return 0;
}

int CPCAPFile::close()
{
    if (!isValid()) return -1;
    int z = munmap(m_buf, m_fileSize);
    CloseHandle(m_fileHandle);

    return z;
}

int CPCAPFile::flush()
{
    if (!isValid())   return -1;

    return msync(m_buf, g_env->m_config.storage.fileSize, 0);
}
