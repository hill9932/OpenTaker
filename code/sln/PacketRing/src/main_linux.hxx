#ifdef LINUX
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fstream>
#include <map>
#include <boost/interprocess/sync/named_semaphore.hpp> 

using namespace boost::interprocess; 


byte* CreateBlockMemory(const tchar* _name, u_int64 _size)
{
    //
    // keep the name and address map
    //
    static std::map<CStdString, void*>  s_addrMap;
    if (s_addrMap.count(_name) == 0) 
    {
        s_addrMap[_name] = (byte*)AlignedAlloc((_size / PAGE_SIZE + 1) * PAGE_SIZE, PAGE_SIZE); //new byte[_size];
    }
    return (byte*)s_addrMap[_name];


    CStdStringA shareName = "/dev/shm/";
    shareName += _name;
    std::ofstream of;
    of.open(shareName, ios_base::out | ios_base::trunc);
    of.close();

    key_t key = ftok(shareName, 0);
    if (key == -1) 
    {
        RM_LOG_ERROR("ftok failed: " << strerror(errno));
        return NULL;
    }
    ON_ERROR_PRINT_LASTMSG_AND_DO(key, ==, -1, return NULL);

    int flag = 0;//SHM_HUGETLB; // try huge page first
retry:
    int shmID = shmget(key, _size, flag | IPC_CREAT | 0666);	
    if (shmID == -1) 
    {
        if (flag)
        {
            flag = 0;
            goto retry;
        }
        RM_LOG_ERROR("shmget failed (" << _name << "): " << strerror(errno));
        return NULL;
    }

    ON_ERROR_PRINT_LASTMSG_AND_DO(shmID, ==, -1, return NULL);

    byte* addr = (byte*)shmat(shmID, NULL, 0);
    ON_ERROR_PRINT_LASTMSG_AND_DO(addr, ==, NULL, return NULL);

    return addr;
}

extern "C"
bool UninitRing(PktRingHandle_t _handler, ModuleID _moduleId, const char* _globalName)
{
    return true;

    if (!_globalName)   _globalName = SHARE_MEM_GLOBAL_NAME;
    CStdString blockName = _globalName;
    blockName += "_BlockSM";
    CStdString metaName = blockName;
    metaName += "_MetaSM";
    CStdString metaBlkName = _globalName;
    metaBlkName += "_MetaBlockSM";
    CStdString shareInfoName = _globalName;
    shareInfoName += "_SharedInfo";

    const char* shareNames[] = 
    {
        _globalName,
        shareInfoName.c_str(),
        blockName.c_str(),
        metaName.c_str(),
        metaBlkName.c_str()
    };

    Global_t *_G = (Global_t *)_handler;
    if (_G) 
    {
        if (_G->g_moduleInfo[_moduleId].blockViewStart)
            shmdt(_G->g_moduleInfo[_moduleId].blockViewStart);
        if (_G->g_moduleInfo[_moduleId].metaViewStart)
            shmdt(_G->g_moduleInfo[_moduleId].metaViewStart);
        if (_G->g_moduleInfo[_moduleId].metaBlkViewStart)
            shmdt(_G->g_moduleInfo[_moduleId].metaBlkViewStart);
        if (_G->g_moduleInfo[_moduleId].sharedInfoViewStart)
            shmdt(_G->g_moduleInfo[_moduleId].sharedInfoViewStart);

        shmdt((void*)_G);
    }

    for (unsigned int i = 0; i < sizeof(shareNames) / sizeof(shareNames[0]); ++i)
    {
        CStdStringA shareName = "/dev/shm/";
        shareName += shareNames[i];

        key_t key = ftok(shareName, 0);
        ON_ERROR_PRINT_LASTMSG_AND_DO(key, ==, -1, return false);
        
        int shmID = shmget(key, 0, 0);	
        ON_ERROR_PRINT_LASTMSG_AND_DO(shmID, ==, -1, return false);

        struct shmid_ds shmStat = {0};
        int z = shmctl(shmID, IPC_STAT, &shmStat);
        ON_ERROR_PRINT_LASTMSG_AND_DO(z, !=, 0, return false);

        if (shmStat.shm_nattch == 0)
        {
            z = shmctl(shmID, IPC_RMID, NULL);
            ON_ERROR_PRINT_LASTMSG_AND_DO(z, !=, 0, return false);

            unlink(shareName);
        }
    }

    return true;
}

int TrigReadySignal(const char* _signal)
{
    if (!_signal) return -1;
    try
    {
        named_semaphore sem(open_only_t(), _signal);
        sem.post();
    }
    catch(interprocess_exception& ex)
    {
        RM_LOG_ERROR("Fail to wait signal " << _signal << ": " << ex.what());
        return -1;
    }

    return 0;
}


#endif
