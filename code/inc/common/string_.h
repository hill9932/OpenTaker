#ifndef __HILUO_STRING_INCLUDE_H__
#define __HILUO_STRING_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    /**
     * @Function: Split string specified by delim and put the substring into vector
     **/
    int StrSplit(const tchar* _src, const tchar* _delim, vector<CStdString>& _vec);
    int StrSplit(const CStdString& _src, const tchar* _delim, vector<CStdString>& _vec);

    /**
     * @Function: Splite string by substring
     **/
    int StrSplit_s(const tchar* _src, const tchar* _delim, vector<CStdString>& _vec);
    int StrSplit_s(const CStdString& _src, const tchar* _delim, vector<CStdString>& _vec);

    /**
     * @Function: Remove the space chars at the front and end of a string
     **/
    tchar* StrTrim(tchar* _src);

    /**
     * @Function: Convert a command string into argv[]
     * @Param _cmd: Contain the execute command string, such as "cmd.exe param1"
     * @Param _argv: User allocated array of pointer
     * @Param _argc: the size of _argv
     * @Return the duplicated buffer, should be freed
     **/
    tchar* Str2Argv(const tchar* _cmd, tchar** _argv, int& _argc);

    /**
     * @Function:¡¡Get the file path exclude the command line
     **/
    CStdString GetFileName(const CStdString& _path);
    CStdString GetFileName(const tchar* _path);

    /**
     * @Function: Return the value of hexidecimal char
     **/
    int GetXValue(char _x);

    /**
    * @Function: translate the "0x1234EF" to string
    **/
    void Bin2Str(const byte* _p, char* _to, size_t _len, bool _upperCase);

    /**
     * @Function: translate the "0xFF" to number
     **/
    byte AtoX(const char* _xcode);

    u_int32 HashString(const tchar* _name);

}

#endif

