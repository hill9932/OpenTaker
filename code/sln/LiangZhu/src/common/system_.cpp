#include "system_.h"
#include "string_.h"
#include "file_system.h"

#ifdef LINUX
#include <sys/vfs.h>
#include <dlfcn.h>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/interprocess/sync/named_semaphore.hpp" 
using namespace boost::interprocess;
#endif

namespace LiangZhu
{

#ifdef WIN32

    void YieldCurThread()
    {
        SwitchToThread();
    }

    PID_T GetCurrentPID()
    {
        return GetCurrentProcessId();
    }

    u_int32 GetCurrentThreadID()
    {
        return GetCurrentThreadId();
    }

    process_t LaunchProcess(tchar* _path)
    {
        if (!_path) return NULL;

        CStdString path(_path);
        return LaunchProcess(path);
    }

    process_t LaunchProcess(CStdString& _path)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Start the child process. 
        BOOL b = CreateProcess(NULL,   // No module name (use command line)
            (LPSTR)_path.c_str(),          // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,//CREATE_NEW_CONSOLE,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi);          // Pointer to PROCESS_INFORMATION structure

        ON_ERROR_LOG_MESSAGE_AND_DO(b, == , false, _path, 1);
        return pi.hProcess;
    }

    process_t LaunchProcess_l(tchar* _path)
    {
        if (!_path) return NULL;

        CStdString path(_path);
        return LaunchProcess_l(path);
    }

    process_t LaunchProcess_l(CStdString& _path)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Start the child process. 
        BOOL b = CreateProcess(_path,   // No module name (use command line)
            NULL,          // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,//CREATE_NEW_CONSOLE,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi);          // Pointer to PROCESS_INFORMATION structure

        ON_ERROR_LOG_MESSAGE_AND_DO(b, == , false, _path, 0);
        return pi.hProcess;
    }

    int WaitProcess(process_t _process, u_int32 _timeoutMs)
    {
        return WaitForSingleObject(_process, _timeoutMs);
    }

    int KillProcess(process_t _process)
    {
        return TerminateProcess(_process, -1);
    }

    handle_t CreateSignal(const tchar* _signal)
    {
        if (!_signal) return NULL;
        return CreateEvent(NULL, FALSE, FALSE, _signal);
    }

    int RemoveSignal(const tchar* _signal)
    {
        return 0;
    }

    int WaitSignal(const tchar* _signal, u_int32 _timeoutMs)
    {
        if (!_signal) return -1;
        handle_t hSig = CreateSignal(_signal);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(hSig, == , NULL, return err);

        int ret = WaitForSingleObject(hSig, _timeoutMs);
        CloseHandle(hSig);

        return ret;
    }

    int TryWaitSignal(const tchar* _signal)
    {
        if (!_signal) return -1;
        handle_t hSig = CreateSignal(_signal);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(hSig, == , NULL, return err);

        int ret = WaitForSingleObject(hSig, 0);
        CloseHandle(hSig);

        return ret;
    }


    int TrigSignal(const tchar* _signal)
    {
        if (!_signal) return -1;
        handle_t hSig = CreateSignal(_signal);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(hSig, == , NULL, return err);

        SetEvent(hSig);
        return 0;
    }

    int ListDir(const tchar* _dirName, vector<CStdString>& _names)
    {
        if (!_dirName)  return -1;

        WIN32_FIND_DATA fileData;
        handle_t hFind = FindFirstFile(_dirName, &fileData);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(hFind, == , INVALID_HANDLE_VALUE, return err);

        do
        {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            }
            else
            {
                _names.push_back(fileData.cFileName);
            }
        } while (FindNextFile(hFind, &fileData) != 0);

        FindClose(hFind);
        return 0;
    }

    u_int64 GetAvailableSpace(const tchar* _disk)
    {
        if (!_disk) return 0;

        ULARGE_INTEGER freeBytes = { 0 };
        BOOL b = GetDiskFreeSpaceEx(_disk, &freeBytes, NULL, NULL);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(b, == , false, return 0);
        if (!b) RM_LOG_ERROR(_disk);

        return freeBytes.QuadPart >> 30;    // GB
    }

    u_int64 GetAvailableSpace(const CStdString& _disk)
    {
        return GetAvailableSpace(_disk.c_str());
    }


#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

    int GetSysNICs2(vector<DeviceST>& _NICs)
    {
        char buf[4096 * 4] = { 0 };
        DWORD dwRetVal = 0;
        unsigned int i = 0;
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
        ULONG family = AF_INET;
        PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)&buf;
        PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
        ULONG outBufLen = sizeof (IP_ADAPTER_ADDRESSES);

        outBufLen = sizeof (IP_ADAPTER_ADDRESSES);
        pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);

        // Make an initial call to GetAdaptersAddresses to get the 
        // size needed into the outBufLen variable
        if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
        {
            free(pAddresses);
            pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
        }

        int devCount = 0;
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == NO_ERROR)
        {
            pCurrAddresses = pAddresses;
            while (pCurrAddresses)
            {
                if (pCurrAddresses->TransmitLinkSpeed != -1 &&
                    pCurrAddresses->PhysicalAddressLength > 0 &&
                    pCurrAddresses->FirstUnicastAddress &&
                    pCurrAddresses->TransmitLinkSpeed / 1000000 >= 100)
                {
                    if (pCurrAddresses->IfType == IF_TYPE_IEEE80211 ||
                        pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
                    {
                        DeviceST device;
                        device.linkSpeed = 0;

                        device.name = pCurrAddresses->AdapterName;
                        device.desc = pCurrAddresses->Description;
                        if (pCurrAddresses->TransmitLinkSpeed != -1)
                            device.linkSpeed = (u_int32)(pCurrAddresses->TransmitLinkSpeed / 1000000);

                        pUnicast = pCurrAddresses->FirstUnicastAddress;
                        if (pUnicast != NULL)
                        {
                            for (i = 0; pUnicast != NULL; i++)
                            {
                                pUnicast = pUnicast->Next;
                            }
                        }

                        if (pCurrAddresses->PhysicalAddressLength != 0)
                        {
                            for (i = 0; i < pCurrAddresses->PhysicalAddressLength; i++)
                            {
                                char buf[128] = { 0 };
                                if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                                    sprintf_t(buf, TEXT("%.2X"), (int)pCurrAddresses->PhysicalAddress[i]);
                                else
                                    sprintf_t(buf, TEXT("%.2X-"), (int)pCurrAddresses->PhysicalAddress[i]);

                                device.portStat[0].mac += buf;
                            }
                        }

                        device.portStat[0].status = LINK_ON;
                        device.myDevIndex = devCount;
                        device.myPortIndex = devCount;
                        _NICs.push_back(device);

                        ++devCount;
                    }
                }

                pCurrAddresses = pCurrAddresses->Next;
            }
        }

        if (pAddresses) free(pAddresses);

        return _NICs.size();
    }

    int GetSysNICs(vector<DeviceST>& _NICs)
    {
        PIP_ADAPTER_INFO pAdapterInfo;
        PIP_ADAPTER_INFO pAdapter = NULL;
        DWORD dwRetVal = 0;
        UINT i;

        ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof (IP_ADAPTER_INFO));

        //
        // Make an initial call to GetAdaptersInfo to get
        // the necessary size into the ulOutBufLen variable
        //
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
        {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
        }

        if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
        {
            pAdapter = pAdapterInfo;
            while (pAdapter)
            {
                if (pAdapter->Type == MIB_IF_TYPE_LOOPBACK)
                {
                    pAdapter = pAdapter->Next;
                    continue;
                }

                DeviceST device;
                device.name = pAdapter->AdapterName;
                device.desc = pAdapter->Description;
                device.linkSpeed = 1000;

                for (i = 0; i < pAdapter->AddressLength; i++)
                {
                    char buf[128] = { 0 };
                    if (i == (pAdapter->AddressLength - 1))
                        sprintf_t(buf, TEXT("%.2X"), (int)pAdapter->Address[i]);
                    else
                        sprintf_t(buf, TEXT("%.2X-"), (int)pAdapter->Address[i]);

                    device.portStat[0].mac += buf;
                }
                /*
                DeviceAddrST addr;

                printf("\tIP Address: \t%s\n",
                pAdapter->IpAddressList.IpAddress.String);
                printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);

                printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
                */
                _NICs.push_back(device);
                pAdapter = pAdapter->Next;
            }
        }

        if (pAdapterInfo)
            free(pAdapterInfo);

        return _NICs.size();
    }

    HMODULE LoadLibrary(const tchar* _path, int _flag)
    {
        return ::LoadLibrary(_path);
    }


    void* GetFuncAddress(HMODULE _module, const tchar* _funName)
    {
        if (!_funName)  return NULL;
        return GetProcAddress(_module, _funName);
    }

#else

    void YieldCurThread()
    {
        pthread_yield();
    }

    PID_T GetCurrentPID()
    {
        return getpid();
    }

    u_int32 GetCurrentThreadID()
    {
        return pthread_self();
    }

    process_t LaunchProcess(tchar* _path)
    {
        if (!_path) return (process_t)NULL;

        CStdString path(_path);
        return LaunchProcess(path);
    }

    process_t LaunchProcess(CStdString& _path)
    {
        pid_t pid = fork();
        if (pid == 0)   // child process
        {
            tchar* argv[64] = {0};
            int argc = 64;

            Str2Argv(_path.c_str(), argv, argc);

            int z = execv(argv[0], argv);  
            ON_ERROR_LOG_MESSAGE_AND_DO(z, !=, 0, _path, "");

            if (z != 0)
            {
                z = execvp(_path.c_str(), argv);
                ON_ERROR_LOG_MESSAGE_AND_DO(z, !=, 0, _path, "");
            }

            exit(0);
        }

        return (handle_t) pid;
    }

    PID_T WaitProcess(int* _status)  
    {  
        int revalue;  
        while (((revalue = wait(_status)) == -1))
        {
            int err = GetLastSysError();
            LOG_ERROR_MSG(err);
            if (err != EINTR)
                break;
        }

        return revalue;  
    }  

    int WaitProcess(process_t _process,  u_int32 _timeoutMs)
    {
        bool retry = true;

        while (retry)
        {
            int ret = waitpid(_process, NULL, WNOHANG);
            if (ret == _process)   return 0;

            SleepMS(10);
            _timeoutMs -= 10;
            retry = (_timeoutMs > 10);
        }

        return -1;
    }

    int KillProcess(process_t _process)
    {
        char cmd[128] = {0};
        sprintf_t(cmd, "kill -9 %d", _process);
        return system(cmd); 
    }

    int MySystem(const tchar* _cmdString, char* _buf, int _len)
    {
        if (!_cmdString)    return -1;

        int fd[2]; 
        int n, count;
        if (_buf)   memset(_buf, 0, _len);
        if (pipe(fd) < 0)       return -1;

        pid_t pid;
        if ((pid = fork()) < 0) 
            return -1;
        else if (pid > 0)     // parent process
        {
            close(fd[1]);     // close write end 
            count = 0;
            while (_buf && (n = read(fd[0], _buf + count, _len)) > 0 && count > _len)
                count += n;
            close(fd[0]);
            if (waitpid(pid, NULL, 0) > 0)
                return -1;
        }
        else    /* child process */
        {
            close(fd[0]);     /* close read end */
            if (fd[1] != STDOUT_FILENO)
            {
                if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
                {
                    return -1;
                }
                close(fd[1]);
            }
            if (execl("/bin/sh", "sh", "-c", _cmdString, (char*)0) == -1)
                return -1;
        }

        return 0;
    }

    handle_t CreateSignal(const tchar* _signal)
    {
        if (!_signal) return NULL;
        ::mode_t mask = umask(0);

        try
        {
            named_semaphore sem(open_or_create_t(), _signal, 0);
        }
        catch(interprocess_exception& ex)
        {
            RM_LOG_ERROR("Fail to create signal '" << _signal << "': " << ex.what());
            umask(mask);
            return (handle_t) 0;
        }

        umask(mask);
        return (handle_t) 1;
    }

    int RemoveSignal(const tchar* _signal)
    {
        if (!_signal) return NULL;
        bool b = false;
        try
        {
            b = !named_semaphore::remove(_signal);
        }
        catch(interprocess_exception& ex)
        {
            RM_LOG_ERROR("Fail to remove signal '" << _signal << "': " << ex.what());
            return -1;
        }

        return 0;
    }

    int WaitSignal(const tchar* _signal, u_int32 _timeoutMs)
    {
        if (!_signal) return -1;
        try
        {
            named_semaphore sem(open_only_t(), _signal);

            boost::posix_time::ptime timeout =  boost::posix_time::microsec_clock::universal_time();
            timeout += boost::posix_time::milliseconds(_timeoutMs);
            bool b = sem.timed_wait(timeout);   // true when signaled, false when timeout, exception when error

            return !b;  // 0 will be success
        }
        catch(interprocess_exception& ex)
        {
            RM_LOG_ERROR("Fail to wait signal '" << _signal << "': " << ex.what());
        }

        return -1;
    }


    int TryWaitSignal(const tchar* _signal)
    {
        if (!_signal) return -1;
        try
        {
            named_semaphore sem(open_only_t(), _signal);
            bool b = sem.try_wait();   // true when signaled, false when timeout, exception when error

            return !b;  // 0 will be success
        }
        catch(interprocess_exception& ex)
        {
            RM_LOG_ERROR("Fail to wait signal '" << _signal << "': " << ex.what());
        }

        return -1;
    }


    int TrigSignal(const tchar* _signal)
    {
        if (!_signal) return -1;

        try
        {
            named_semaphore sem(open_only_t(), _signal);
            sem.post();
            return 0;
        }
        catch(interprocess_exception& ex)
        {
            RM_LOG_ERROR("Fail to trig signal '" << _signal << "': " << ex.what());
        }

        return -1;
    }

    u_int64 GetAvailableSpace(const tchar* _disk)
    {
        if (!_disk) return 0;

        struct statfs64 diskInfo;        
        statfs64(_disk, &diskInfo);  
        u_int64 blocksize = diskInfo.f_bsize;   
        u_int64 totalsize = blocksize * diskInfo.f_bavail; 
        return totalsize >> 30;     // GB
    }

    u_int64 GetAvailableSpace(const CStdString& _disk)
    {
        return GetAvailableSpace(_disk.c_str());
    }

    int ListDir(const tchar* _dirName, vector<CStdString>& _names)
    {
        if (!_dirName)  return -1;

        DIR *p_dir = NULL;
        struct dirent *p_dirent = NULL;
        p_dir = opendir(_dirName);
        ON_ERROR_RETURN_LAST_ERROR(p_dir, ==, NULL);

        while ((p_dirent = readdir(p_dir)))
        {
            if (p_dirent->d_name[0] != '.')
                _names.push_back(p_dirent->d_name);
        }
        closedir(p_dir);
        return 0;
    }

    HMODULE LoadLibrary(const tchar* _path, int _flag)
    {
        if (!_path) return NULL;
        HMODULE h= dlopen(_path, RTLD_NOW);
        if (h == NULL)
            RM_LOG_ERROR(dlerror());

        return h;
    }

    void* GetFuncAddress(HMODULE _module, const tchar* _funName)
    {
        if (!_funName)  return NULL;
        return dlsym(_module, _funName);
    }
    
#endif

}
