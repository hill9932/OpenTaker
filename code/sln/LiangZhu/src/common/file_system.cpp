#include "file_system.h"
#include "file.h"
#include <sys/stat.h>
#include <fcntl.h>
#ifdef LINUX
#include <libgen.h>
#endif

bool IsFileExist(const tchar* _path)
{
    assert(_path);
    if (!_path)  return false;

    struct stat_ fileStat = { 0 };
    int z = stat_t(_path, &fileStat);
    if (0 != z) return false;

    return (fileStat.st_mode & S_IFREG) > 0;
}

bool IsFileExist(const CStdString& _path)
{
    return IsFileExist(_path.c_str());
}

bool IsDirExist(const tchar* _path)
{
    assert(_path);
    if (!_path)  return false;

    struct stat_ fileStat = { 0 };
    int z = stat_t(_path, &fileStat);
    if (0 != z) return false;

    return (fileStat.st_mode & S_IFDIR) > 0;
}

bool IsDirExist(const CStdString& _path)
{
    return IsDirExist(_path.c_str());
}

bool CanFileAccess(const tchar* _path)
{
    assert(_path);
    if (!_path)  return false;

    return access_t(_path, 0) == 0;
}

bool CanFileAccess(const CStdString& _path)
{
    return CanFileAccess(_path.c_str());
}

bool CanDirAccess(const tchar* _path)
{
    if (!_path) return false;
    CStdString tmpFile = _path;
    tmpFile += "/access_detect.txt";

#ifndef WIN32
    int h = ::open(tmpFile, O_CREAT | O_RDWR, 0644);
    if (h == -1)    return false;
    ::close(h);
    unlink(tmpFile);
#endif

    return true;
}

CStdString GetAppDir()
{
    tchar appPath[MAXPATH] = {0};

#ifdef WIN32
    DWORD dwSize = GetModuleFileName(NULL, appPath, MAXPATH-1);
#else
    //getcwd(appPath, MAXPATH);
    //strcat(appPath, "/");

    int rval = readlink ("/proc/self/exe", appPath, sizeof(appPath));
    if (rval == -1)
        return "";
    else
        appPath[rval] = '\0';
#endif

    return GetFilePath(appPath) + "/";
}

CStdString GetFileName(const tchar* _path)
{
    assert(_path);
    if (!_path) return "";
    return GetFileName(CStdString(_path));
}

CStdString GetFileName(const CStdString& _path)
{
    int ending = _path.size() - 1;
    while (ending >= 0 &&
        (_path.GetAt(ending) == '/' ||
        _path.GetAt(ending) == '\\'))
        --ending;

    int pos = -1;
    if ((pos = _path.ReverseFind('\\')) != (size_t)-1 ||
        (pos = _path.ReverseFind('/'))  != (size_t)-1)
    {
        return ending >= pos ? _path.Mid(pos+1) : "";
    }

    return "";
}

CStdString GetFilePath(const CStdString& _path)
{
    int pos = _path.size() - 1;
    while (pos >= 0 && 
          (_path.GetAt(pos) == '/' || 
           _path.GetAt(pos) == '\\'))
        --pos;

    //
    // only keep one '/' or '\' at the ending
    //
    if (pos != _path.size() - 1)
    {
        return _path.Mid(0, MyMax(1, pos+1));
    }

    size_t prexPos = 0;
    while (_path.GetAt(prexPos) == '/' || _path.GetAt(prexPos) == '\\')
        ++prexPos;

    if ((pos = _path.ReverseFind('\\')) != (size_t)-1 ||
        (pos = _path.ReverseFind('/'))  != (size_t)-1)
    {
        if (pos > prexPos)
            return _path.Mid(0, pos);
        else
            return "";
    }

    return "";
}

CStdString GetFilePath(const tchar* _path)
{
    assert(_path);
    if (!_path) return "";
    return GetFilePath(CStdString(_path));
}

bool CreateFilePath(const tchar* _filePath)
{
    if (!_filePath) return false;
    
    CStdString  fileDir = GetFilePath(_filePath);
    if (!CreateAllDir(fileDir))
        return false;

    int h = ::open(_filePath, O_RDWR | O_CREAT, 0644);
    if (h == -1)
        return false;

    close(h);
    return true;
}

bool CreateAllDir(const tchar* _path)
{
    assert(_path);
    if (!_path) return false;

    if (IsDirExist(_path))    return true;

    AutoFree<tchar> buf(new tchar[strlen_t(_path) + 1], DeleteArray);
    strcpy_t(buf, _path);

    tchar  dir_name[256] = {0};
    tchar* p = buf;
    tchar* q = dir_name;

    while (*p)
    {
        if (('\\' == *p) || ('/' == *p))
        {
            if (':' != *(p-1))
            {
                mkdir(dir_name, DEFAULT_RIGHT);
            }
        }
        *q++ = *p++;
        *q = '\0';
    }

    int z = mkdir(dir_name, DEFAULT_RIGHT);
#ifdef WIN32
    return z || (GetLastSysError() == 0xb7);
#else
    return z == 0;
#endif
}

bool CreateAllDir(const CStdString& _path)
{
    return CreateAllDir(_path.c_str());
}

bool DeletePath(const tchar* _path)
{
    assert(_path);
    if (!_path) return false;

    //
    // when _path is a file, call remove
    // when it is a directory, call rmdir
    //
    struct stat_ fileStat = { 0 };
    int z = stat_t(_path, &fileStat);
    if (0 != z) return false;

    if (fileStat.st_mode & S_IFDIR)
        return 0 == rmdir_t(_path);
    else
        return 0 == remove_t(_path);

    return false;
}

bool DeletePath(const CStdString& _path)
{
    return DeletePath(_path.c_str());
}

bool RemoveDir(const tchar* _path)
{
    CStdString cmd = "rm -fr ";
    cmd += _path;
    int z = system(cmd);
    return z == 0;
}

#ifdef LINUX
#include <xfs/xfs.h>
#endif

bool AllocateFile(const tchar* _path, u_int64 _fileSize)
{
    if (IsFileExist(_path)) return true;
    bool isXFS = false;

    int fd = ::open(_path, O_RDWR | O_CREAT, 0644);
    ON_ERROR_LOG_MESSAGE_AND_DO(fd, == , (int)-1, _path, return false);

#ifdef LINUX
    if (platform_test_xfs_fd(fd) == 0)
        isXFS = true;
#endif

    lseek64(fd, _fileSize, SEEK_SET);

#ifdef WIN32
    HANDLE hfile = (HANDLE)_get_osfhandle(fd);
    SetEndOfFile(hfile);

#elif defined(LINUX)
    ftruncate(fd, _fileSize);
    if (isXFS)
    {
        xfs_flock64_t flag = { 0 };
        flag.l_whence = SEEK_SET;
        flag.l_start = 0;
        flag.l_len = _fileSize;
        xfsctl(_path, fd, XFS_IOC_RESVSP64, &flag);
    }
    else
    {
        posix_fallocate(fd, 0, _fileSize);
    }
#endif
    close(fd);

    return true;
}

u_int64 HashFile(const tchar* _path)
{
    if (!_path || !IsFileExist(_path)) return -1;

    CHFile file;
    u_int64 hashValue = 0;
    if (0 != file.open(ACCESS_READ, 0, false, false))   return -1;
    u_int32 fileSize = file.getSize();
    hashValue = fileSize;
    hashValue <<= 32;

    u_int32 data = 0;
    for (int i = 3; i <= 103; ++i)
    {
        u_int64 offset = fileSize / i;

        file.read_w((byte*)&fileSize, sizeof(data), offset);
        hashValue ^= data;
    }

    return hashValue;
}
