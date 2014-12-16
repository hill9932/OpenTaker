#ifndef __HILUO_STORAGE_INCLUDE_H__
#define __HILUO_STORAGE_INCLUDE_H__

#include "common.h"

enum WriteMode_e
{
    WRITE_SEQUENTIAL = 1,
    WRITE_INTERLEAVED
};

const int g_filesPerDir = 128;  // max file count in a directory


/**
* When multiple target config, and write mode could be sequential or interleaved
* these function used to adjust the file info according to the write mode
**/
CStdString GetFileSubDirByIndex(u_int32 _index, u_int32 _dirLevel);
CStdString GetFilePathByIndex(u_int32 _index, u_int32 _dirLevel);
int AdjustFileIndex(u_int32 _target, int& _fileIndex);
int GetMaxFileIndex(u_int32 _target);
int GetTargetDBIndex(u_int32 _target);
int GetTargetByFileIndex(int& _index);
int GetRecordDBCount();

#endif
