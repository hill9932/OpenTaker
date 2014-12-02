/**
 * @Function: 
 *  Define function related to os, such as CPU usage
 * @Memo:
 *  Create by hill, 5/16/2014
 **/

#ifndef __VP_SYSTEM_INCLUDE_H__
#define __VP_SYSTEM_INCLUDE_H__

#include "common.h"
#include "device.h"

PID_T   GetCurrentPID();
u_int32 GetCurrentThreadID();

int DropPrivileges(const char* _username = "nobody");

/**
 * @Function: Get the NIC on the local system
 **/
int GetSysNICs(vector<DeviceST>& _NICs);
int GetSysNICs2(vector<DeviceST>& _NICs);

/**
 * @Function: Get the disk space in GB
 **/
u_int64 GetAvailableSpace(const tchar* _disk);
u_int64 GetAvailableSpace(const CStdString& _disk);

/**
 * @Function: Launch the specified process, it will search the
 *  process in multiple location.
 * @Return:
 *  handle_t: the handle of new process
 *  NULL
 **/
process_t LaunchProcess(tchar* _path);
process_t LaunchProcess(CStdString& _path);

/**
 * @Function: Launch the specified process, it will only execute the
 *  process under current directory.
 **/
process_t LaunchProcess_l(tchar* _path);
process_t LaunchProcess_l(CStdString& _path);

/**
 * @Function: Wait for any child process to end.
 * @Return pid_t: the terminated child process id
 * @Return -1: error
 **/
PID_T WaitProcess(int* _status);
int   WaitProcess(process_t _process,  u_int32 _timeoutMs = INFINITE);

int   KillProcess(process_t _process);
void  YieldCurThread();

/**
 * @Function: Exec a shell command and capture the command output
 **/
int MySystem(const tchar* _cmdString, char* _buf, int _len);
int ListDir(const tchar* _dirName, vector<CStdString>& _names);

/**
 * @Function: signal operation
 * @Return 0: signal is triggered.
 **/
int      WaitSignal(const tchar* _signal, u_int32 _timeoutMs = INFINITE);
int      TryWaitSignal(const tchar* _signal);
int      TrigSignal(const tchar* _signal);
int      RemoveSignal(const tchar* _signal);
handle_t CreateSignal(const tchar* _signal);

/**
 * @Function: Get the function of address on the specified module.
 **/
HMODULE LoadLibrary(const tchar* _path, int _flag);
void* GetFuncAddress(HMODULE _module, const tchar* _funName);

#endif
