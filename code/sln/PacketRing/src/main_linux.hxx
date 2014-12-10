#ifdef LINUX
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fstream>
#include <boost/interprocess/sync/named_semaphore.hpp> 

using namespace boost::interprocess; 


byte* CreateBlockMemory(const tchar* _name, u_int64 _size)
{
    CStdStringA shareName = "/dev/shm/";
    shareName += _name;
    std::ofstream of;
    of.open(shareName, ios_base::out | ios_base::trunc);
    of.close();

    key_t key = ftok(shareName, 0);
    if (key == -1) 
    {
        printf("ftok failed: %s", strerror(errno));
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
        printf("shmget failed: %s %s " I64D, strerror(errno), _name, _size);
        return NULL;
    }

    ON_ERROR_PRINT_LASTMSG_AND_DO(shmID, ==, -1, return NULL);

    byte* addr = (byte*)shmat(shmID, NULL, 0);
    if (addr == NULL)
    {   
        printf("shmat failed: %s %s " I64D, strerror(errno), _name, _size);
        return NULL;
    }
    ON_ERROR_PRINT_LASTMSG_AND_DO(addr, ==, NULL, return NULL);

    return addr;
}

bool ReleaseAgent(PktRingHandle_t _handler, ModuleID _moduleId, const char* _globalName)
{
    if (!_globalName)   _globalName = SHARE_MEM_GLOBAL_NAME;
    CStdString blockName = _globalName;
    blockName += "_BlockSM";
    CStdString metaName = blockName;
    metaName += "_MetaSM";
    CStdString metaBlkName = _globalName;
    metaBlkName += "_MetaBlockSM";

    const char* shareNames[] = 
    {
        _globalName,
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

        bzero(_G, sizeof(Global_t));
    }

    for (unsigned int i = 0; i < sizeof(shareNames) / sizeof(shareNames[0]); ++i)
    {
        CStdStringA shareName = "/dev/shm/";
        shareName += shareNames[i];

        key_t key = ftok(shareName, 0);
        if (key == -1)      return false;

        int shmID = shmget(key, 0, IPC_CREAT | 0666);	
        if (shmID == -1)    return false;

        int z = shmctl(shmID, IPC_RMID, NULL);
        if (z != 0) return false;

        unlink(shareName);
    }

    StopAgent(_handler);

    return true;
}

#endif
