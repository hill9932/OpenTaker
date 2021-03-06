#ifndef __HILUO_LUAWRAP_INCLUDE_H__
#define __HILUO_LUAWRAP_INCLUDE_H__

#include "common.h"
#include <map>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

typedef std::map<CStdStringA, CStdStringA>    LuaObject;
typedef int(*lua_CFunction) (lua_State *L);

class CLua
{
public:
    CLua();
    ~CLua();

    /**
    * @Function: load a lua file
    **/
    int     doFile(const tchar* _filePath);
    int     doString(const tchar* _chunk);
    void    dumpStack();

    /**
    * @Function: call a lua function.
    * @Param _func: the name of function to be called
    * @Param _params: the format of parameters and return values.
    *                 like "dd>d" means two double parameters and a double return value
    **/
    int     callFunc(const tchar* _func, const tchar* _params, ...);

    /**
    * @Function: register a function to be called to lua
    **/
    int     regFunc(const tchar* _funcName, lua_CFunction _func);

    /**
    * @Function: register an object.
    **/
    int     regClass(const tchar* _className, luaL_Reg* _classMems);
    char**  makeArgv(const char * _cmd);

    bool    loadObject(const tchar* _tableName, LuaObject& _fields);
    bool    loadRecords(const tchar* _tableName, std::vector<LuaObject>& _records);

    bool    loadArray_s(const tchar* _tableName, std::vector<CStdString>& _vec);
    bool    loadArray_i(const tchar* _tableName, std::vector<int>& _vec);
    bool    loadArray_d(const tchar* _tableName, std::vector<double>& _vec);

    bool    loadBoolean(const tchar* _name, bool& _value);
    bool    loadBoolean(const tchar* _tableName, const tchar* _name, bool& _value);

    bool    loadInteger(const tchar* _name, int& _value);
    bool    loadInteger(const tchar* _tableName, const tchar* _name, int& _value);

    bool    loadDouble(const tchar* _name, double& _value);
    bool    loadDouble(const tchar* _tableName, const tchar* _name, double& _value);

    bool    loadString(const tchar* _name, CStdString& _value);
    bool    loadString(const tchar* _tableName, const tchar* _name, CStdString& _value);

private:
    void error(const char *_fmt, ...);

private:
    lua_State* m_luaState;
};


#endif
