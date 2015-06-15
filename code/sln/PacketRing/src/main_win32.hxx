#ifdef WIN32

byte* CreateBlockMemory(const char* _name, u_int64 _size)
{
    if (!_name) return NULL;

    static std::map<CStdString, void*>  s_addrMap;
    if (s_addrMap.count(_name) == 0)
    {
        s_addrMap[_name] = (byte*)AlignedAlloc((_size / PAGE_SIZE + 1) * PAGE_SIZE, PAGE_SIZE); //new byte[_size];
    }
    return (byte*)s_addrMap[_name];

    handle_t memObj = CreateFileMapping(INVALID_HANDLE_VALUE,
        NULL, 
        PAGE_READWRITE | SEC_COMMIT, 
        _size >> 32, 
        _size, 
        _name);  
    if (!memObj)  return NULL;

    byte* lpData = (byte*)MapViewOfFile(memObj, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);  
    if (lpData == NULL)  
    {  
        CloseHandle(memObj);  
        return NULL;  
    }  

    return lpData;
}

extern "C"
bool UninitRing(PktRingHandle_t _handler, ModuleID _moduleId, const tchar* _globalName)
{
    return true;

    Global_t *_G = (Global_t *)_handler;

    // TODO: CloseHandle the file mapping handle

    if (_G)
    {
        if (_G->g_moduleInfo[_moduleId].blockViewStart)
            UnmapViewOfFile(_G->g_moduleInfo[_moduleId].blockViewStart);
        if (_G->g_moduleInfo[_moduleId].metaViewStart)
            UnmapViewOfFile(_G->g_moduleInfo[_moduleId].metaViewStart);
        if (_G->g_moduleInfo[_moduleId].metaBlkViewStart)
            UnmapViewOfFile(_G->g_moduleInfo[_moduleId].metaBlkViewStart);

        bzero(_G, sizeof(Global_t));
    }

    StopRing(_handler);

    return true;
}

#endif
