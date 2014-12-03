#include "lua_.h"

CLua::CLua()
{
    m_luaState = luaL_newstate();
    luaL_openlibs(m_luaState);
}

CLua::~CLua()
{
    //if (m_luaState)    lua_close(m_luaState);
}

int CLua::doFile(LPCSTR _file_path)
{
    if (!m_luaState)   return -1;

    luaL_openlibs(m_luaState); //Load base libraries
    if (luaL_loadfile(m_luaState, _file_path) || 
        lua_pcall(m_luaState, 0, 0, 0))
    {
        error(lua_tostring(m_luaState, -1));
        lua_pop(m_luaState, 1);
        return -1;
    }

    return 0;
}

int CLua::callFunc(LPCSTR _func, LPCSTR _params, ...)
{
    if (!_func) return -1;
    int nowTop = lua_gettop(m_luaState);

    lua_getglobal(m_luaState, _func); // push function name
    if (!lua_isfunction(m_luaState, -1))
    {
        lua_settop(m_luaState, nowTop);
        return -1;
    }

    va_list args, vl;
    va_start(args, _params);    
    vl = va_arg(args, va_list);

    int nArg = 0, nRes = 0;         // number of arguments and results    
    for (nArg = 0; *_params; nArg++) 
    {
        switch (*_params++) 
        {
            case 'd':   // double argument
                lua_pushnumber(m_luaState, va_arg(vl, double));
                break;
            case 'i':   // int argument
            {
                int v = va_arg(vl, int);
                lua_pushinteger(m_luaState, v);
            }
                break;
            case 's':   // string argument
                lua_pushstring(m_luaState, va_arg(vl, char*));
                break;
            case 'u':   // user data
            {
                void* obj = va_arg(vl, void*);
                lua_pushlightuserdata(m_luaState, obj);        
            }
                break;
            case '>':   // end of arguments
                goto endargs;
            default:
                error("invalid option (%c)", *(_params - 1));
        }
    }

endargs:

    nRes = strlen(_params); // number of expected results
    if (lua_pcall(m_luaState, nArg, nRes, 0) != 0) 
        error("error calling '%s': %s", _func, lua_tostring(m_luaState, -1));

    nRes = -nRes;   // stack index of first result
    while (*_params) 
    {
        switch (*_params++) 
        {
            case 'd':   // double result
            { 
                if (!lua_isnumber(m_luaState, nRes))
                    error("wrong result type");
                else
                {
                    double n = lua_tonumber(m_luaState, nRes);
                    *va_arg(vl, double*) = n;
                }
                break;
            }
            case 'i':   // int result
            {
                if (!lua_isnumber(m_luaState, nRes))
                    error("wrong result type");
                else
                {
                    int n = lua_tointeger(m_luaState, nRes);
                    int *o = va_arg(vl, int*);
                    *o = n;
                }
                break;
            }
            case 's':   // string result
            {
                const char *s = lua_tostring(m_luaState, nRes);
                if (s == NULL)
                    error("wrong result type");
                *va_arg(vl, const char **) = s;
                break;
            }
            case 'u':
            {
                if (!lua_isuserdata(m_luaState, nRes))
                    error("wrong result type");
                else
                {
                    void* ud = lua_touserdata(m_luaState, nRes);
                    *va_arg(vl, void**) = ud;
                }
                break;
            }
            default:
                error("invalid option (%c)", *(_params - 1));
        }
        nRes++;
    }
    
    va_end(vl);
    

    lua_settop(m_luaState, nowTop);

    return 0;
}

int CLua::regFunc(LPCSTR _funName, lua_CFunction _func)
{
    if (!_funName || !_func)  return -1;

    lua_pushcfunction(m_luaState, _func);
    lua_setglobal(m_luaState, _funName);

    return 0;
}

int CLua::regClass(LPCSTR _className, luaL_Reg* _classMems)
{
    if (!m_luaState || !_className || !_classMems) return -1;
    static const luaL_Reg meta[] = { { NULL, NULL } };

    lua_createtable(m_luaState, 0, 0);
    int libID = lua_gettop(m_luaState);

    // metatable = {}
    luaL_newmetatable(m_luaState, _className);
    int metaID = lua_gettop(m_luaState);

    lua_newtable(m_luaState);
    luaL_setfuncs(m_luaState, meta, 0);
 //   luaL_register(m_luaState, NULL, meta);

    // metatable.__index = class_methods
    lua_newtable(m_luaState);
    luaL_setfuncs(m_luaState, _classMems, 0);
    //luaL_register(m_luaState, NULL, _classMems);

    lua_newtable(m_luaState);
    lua_setfield(m_luaState, metaID, "__index");

    // class.__metatable = metatable
    lua_setmetatable(m_luaState, libID);

    // _G["Foo"] = newclass
    lua_setglobal(m_luaState, _className);

    return 0;
}

char** CLua::makeArgv(const char * _cmd)
{
    if (!m_luaState)   return NULL;

    char **argv;
    int i;
    int argc = lua_gettop(m_luaState) + 1;

    if (!(argv = (char**)calloc(argc, sizeof (char *))))
        luaL_error(m_luaState, "Can't allocate memory for arguments array", _cmd);

    argv[0] = (char *)_cmd;
    for (i = 1; i < argc; i++)
    {
        if (lua_isstring(m_luaState, i) || lua_isnumber(m_luaState, i))
        {
            if (!(argv[i] = (char*)lua_tostring (m_luaState, i)))
            {
                luaL_error(m_luaState, "%s - error duplicating string area for arg #%d", _cmd, i);
            }
        }
        else
        {
            luaL_error(m_luaState, "Invalid arg #%d to %s: args must be strings or numbers", i, _cmd);
        }
    }

    return argv;
}


bool CLua::loadObject(LPCSTR _tableName, LuaObject& _obj)
{
    if (!_tableName) return false;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushnil(m_luaState); // the stack£º-1 => nil; -2 => table
    
        while (lua_next(m_luaState, -2))     // the stack£º-1 => value; -2 => key; index => table
        {            
            lua_pushvalue(m_luaState, -2);   // the stack£º-1 => key; -2 => value; -3 => key; index => table
            
            const char* key = lua_tostring(m_luaState, -1);
            const char* value = NULL;
          
            if (lua_isstring(m_luaState, -2))
                value = lua_tostring(m_luaState, -2);
            else if (lua_isboolean(m_luaState, -2))
                value = lua_toboolean(m_luaState, -2) ? "true" : "false";

            transform(key, key + strlen(key), (char*)key, ::tolower);
            if (key && value)   _obj[key] = value;
                        
            lua_pop(m_luaState, 2);   // the stack£º-1 => key; index => table           
        }

/*        LuaObject::iterator it = _fields.begin();
        for(; it != _fields.end(); ++it)
        {
            lua_pushstring(m_state, it->first);       
            lua_gettable(m_state, -2); // now the table is in the -2 and key in the top(-1)
            if (lua_isstring(m_state, -1))
            {
                it->second = lua_tostring(m_state, -1);
            }
            lua_pop(m_state, 1);
        }
*/
        lua_pop(m_luaState, 1);
    }

    lua_pop(m_luaState, 1);

    return true;
}

bool CLua::loadRecords(LPCSTR _tableName, std::vector<LuaObject>& _records)
{
    if (!_tableName) return false;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushnil(m_luaState);  
        lua_gettable(m_luaState, -2);  

        while (lua_next(m_luaState, -2))  
        {  
            lua_pushvalue(m_luaState, -2);  
            const char* key =lua_tostring(m_luaState, -1);  

            if (lua_istable(m_luaState, -2))
            {
                LuaObject record;

                lua_pushnil(m_luaState); 
                lua_gettable(m_luaState, -3);
                while (lua_next(m_luaState, -3))
                {
                    lua_pushvalue(m_luaState, -2);

                    const char* key = lua_tostring(m_luaState, -1);
                    const char* value = "";

                    if (lua_isstring(m_luaState, -2))
                        value = lua_tostring(m_luaState, -2);
                    else if (lua_isboolean(m_luaState, -2))
                        value = lua_toboolean(m_luaState, -2) ? "true" : "false";

                    record[key] = value;

                    lua_pop(m_luaState, 2);
                }

                if (record.size())
                    _records.push_back(record);
            }

            lua_pop(m_luaState, 2);  
        }
    }

    return true;
}

bool CLua::loadArray_s(LPCSTR _tableName, std::vector<CStdString>& _vec)
{
    if (!_tableName) return false;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushnil(m_luaState);        // the stack£º-1 => nil; -2 => table
        while (lua_next(m_luaState, -2))// -1 => value, -2 => key
        {
            if (lua_isstring(m_luaState, -1))
                _vec.push_back(lua_tostring(m_luaState, -1));

            lua_pop(m_luaState, 1);
        }

        return true;
    }

    return false;
}

bool CLua::loadArray_i(LPCSTR _tableName, std::vector<int>& _vec)
{
    if (!_tableName) return false;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushnil(m_luaState);        // the stack£º-1 => nil; -2 => table
        while (lua_next(m_luaState, -2))// -1 => value, -2 => key
        {
            if (lua_isnumber(m_luaState, -1))
                _vec.push_back(lua_tointeger(m_luaState, -1));

            lua_pop(m_luaState, 1);
        }

        return true;
    }

    return false;
}

bool CLua::loadArray_d(LPCSTR _tableName, std::vector<double>& _vec)
{
    if (!_tableName) return false;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushnil(m_luaState);        // the stack£º-1 => nil; -2 => table
        while (lua_next(m_luaState, -2))// -1 => value, -2 => key
        {
            if (lua_isnumber(m_luaState, -1))
                _vec.push_back(lua_tonumber(m_luaState, -1));

            lua_pop(m_luaState, 1);
        }

        return true;
    }

    return false;
}

bool CLua::loadString(LPCSTR _name, CStdString& _value)  
{  
    CStdString  value;
    if (!_name) return false;

    lua_getglobal(m_luaState, _name);  
    if (lua_isstring(m_luaState, -1))  
    {  
        _value = (LPSTR)lua_tostring(m_luaState, -1);  
        return true;
    }  

    return false;  
}  

bool CLua::loadString(LPCSTR _tableName, LPCSTR _name, CStdString& _value)
{
    bool b = false;
    if (!_tableName || !_name)  return b;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushstring(m_luaState, _name);       
        lua_gettable(m_luaState, -2); // now the table is in the -2 and key in the top(-1)
        if (lua_isstring(m_luaState, -1))
        {
            _value = (const char*)lua_tostring(m_luaState, -1);
            b = true;
        }

        lua_pop(m_luaState, 2);
    }

    return b;
}


bool CLua::loadInteger(LPCSTR _name, int& value)  
{  
    if (!_name) return false;

    lua_getglobal(m_luaState, _name);  
    if (lua_isnumber(m_luaState, -1))  
    {  
        value = (int)lua_tointeger(m_luaState, -1);  
        return true;
    }  

    return false;  
}  

bool CLua::loadInteger(LPCSTR _tableName, LPCSTR _name, int& _value)
{
    bool b = false;
    if (!_tableName || !_name)  return b;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushstring(m_luaState, _name);       
        lua_gettable(m_luaState, -2); // now the table is in the -2 and key in the top(-1)
        if (lua_isnumber(m_luaState, -1))
        {
            _value = (int)lua_tointeger(m_luaState, -1);
            b = true;
        }

        lua_pop(m_luaState, 2);
    }

    return b;
}

bool CLua::loadDouble(LPCSTR _name, double& _value)  
{  
    if (!_name) return false;

    lua_getglobal(m_luaState, _name);  
    if (lua_isnumber(m_luaState, -1))  
    {  
        _value = (double)lua_tonumber(m_luaState, -1);  
        return true;
    }  

    return false;  
}  

bool CLua::loadDouble(LPCSTR _tableName, LPCSTR _name, double& _value)
{
    bool b = false;
    if (!_tableName || !_name)  return b;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushstring(m_luaState, _name);       
        lua_gettable(m_luaState, -2); // now the table is in the -2 and key in the top(-1)
        if (lua_isnumber(m_luaState, -1))
        {
            _value = (double)lua_tonumber(m_luaState, -1);
            b = true;
        }

        lua_pop(m_luaState, 2);
    }

    return b;
}

bool CLua::loadBoolean(LPCSTR _name, bool& _value)  
{  
    if (!_name) return false;

    lua_getglobal(m_luaState, _name);  
    if (lua_isboolean(m_luaState, -1))  
    {  
        _value = (bool)lua_toboolean(m_luaState, -1);  
        return true;
    }  
    return false;  
} 

bool CLua::loadBoolean(LPCSTR _tableName, LPCSTR _name, bool& _value)
{
    bool b = false;
    if (!_tableName || !_name)  return b;

    lua_getglobal(m_luaState, _tableName);  
    if (lua_istable(m_luaState, -1))
    {
        lua_pushstring(m_luaState, _name);       
        lua_gettable(m_luaState, -2); // now the table is in the -2 and key in the top(-1)
        if (lua_toboolean(m_luaState, -1))
        {
            _value = (bool)lua_toboolean(m_luaState, -1);
            b = true;
        }

        lua_pop(m_luaState, 2);
    }

    return b;
}

void CLua::dumpStack() 
{
    int i;
    int top = lua_gettop(m_luaState); // depth of the stack
    for (i = 1; i <= top; i++) 
    { 
        int t = lua_type(m_luaState, i);
        switch (t) 
        {
            case LUA_TSTRING: 
                printf("'%s'", lua_tostring(m_luaState, i));
                break;
            case LUA_TBOOLEAN: 
                printf(lua_toboolean(m_luaState, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("%g", lua_tonumber(m_luaState, i));
                break;
            default:
                printf("%s", lua_typename(m_luaState, t));
                break;
        
            printf(" "); /* put a separator */
        }
    }
    printf("\n"); /* end the listing */
}

/** set a new table
    lua_newtable(m_state);
    lua_pushstring(m_state, key); // key
    lua_pushnumber(m_state, 100); // value
    lua_settable(m_state, -3);    // the table is in index 3

    lua_pushnumber(m_state, 200); // value
    lua_setfield(m_state, -2, key);

    lua_setglobal(m_state, "tablename");
 **/

void CLua::error(const char *_fmt, ...) 
{
    va_list argp;
    va_start(argp, _fmt);
    vfprintf(stderr, _fmt, argp);
    va_end(argp);
}
