#ifndef __HILUO_GLOBAL_INCLUDE_H__
#define __HILUO_GLOBAL_INCLUDE_H__

#include "common.h"
using namespace LiangZhu;

class Enviroment
{
public:
    static Enviroment* GetInstance()
    {
        static Enviroment* instance = NULL;
        if (!instance)  instance = new Enviroment;
        return instance;
    }
    Enviroment();
    ~Enviroment();

    bool init(int _argc, char** _argv, const char* _projectName);

    void enable(bool _enable) { m_enable = _enable; }
    bool enable() { return m_enable; }

private:
    bool    m_enable;
};

extern Enviroment* g_env;


static inline 
bool SetupEnviroment(int _argc, char** _argv, const char* _projectName)
{
    g_env = Enviroment::GetInstance();  
    return g_env->init(_argc, _argv, _projectName);
}

#endif
