/**
 * @Function: 
 *  This file contains function interactive with file system, such as file path.
 * @Memo:
 *  Created by hill, 4/11/2014
 **/
#ifndef __HILUO_FILE_SYSTEM_INCLUDE_H__
#define __HILUO_FILE_SYSTEM_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    /**
    * @Function£ºGet the path of execute file in the disk.
    *  There will be a '/' in the end.
    **/
    CStdString GetAppDir();

    /**
     * @Function: Judge whether file exist
     **/
    bool IsFileExist(const tchar* _path);
    bool IsFileExist(const CStdString& _path);

    bool IsDirExist(const tchar* _path);
    bool IsDirExist(const CStdString& _path);

    /**
     * @Function: Judge whether can access the file
     **/
    bool CanFileAccess(const tchar* _path);
    bool CanFileAccess(const CStdString& _path);

    bool CanDirAccess(const tchar* _path);

    /**
     * @Function: Return the file name of a file path
     **/
    CStdString GetFileName(const tchar* _path);
    CStdString GetFileName(const CStdString& _path);

    /**
     * @Function: Get the directory path of a file path
     *  There will be no '\' or '/' in the end
     **/
    CStdString GetFilePath(const tchar* _path);
    CStdString GetFilePath(const CStdString& _path);

    /**
     * @Function: Create all the directories on the path.
     **/
    bool CreateAllDir(const tchar* _path);
    bool CreateAllDir(const CStdString& _path);

    /**
     * @Function: Create the file in the path
     **/
    bool CreateFilePath(const tchar* _filePath);

    /**
     * @Function: Delete the file or directory
     **/
    bool DeletePath(const tchar* _path);
    bool DeletePath(const CStdString& _path);

    bool RemoveDir(const tchar* _path);
    bool AllocateFile(const tchar* _path, u_int64 _fileSize);

    u_int64 HashFile(const tchar* _path);

}

#endif
