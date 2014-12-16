/**
 * @Function:
 *  This file includes functions to handle code convert such as ANSI<->UNICODE
 * @Memo:
 *  Created by hill, 4/11/2014
 **/

#ifndef __HILUO_CODE_CONVERT_INCLUDE_H__
#define __HILUO_CODE_CONVERT_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    /**
     * Function: Adapter for STL algorithm
     **/
    class ToLower
    {
    public:
        int operator()(int _val)
        {
            return tolower(_val);
        }
    };

    class ToUpper
    {
    public:
        int operator()(int _val)
        {
            return toupper(_val);
        }
    };


    void Unicode2Ansi(const wchar_t* in, unsigned int len, std::string& out);
    void Ansi2Unicode(const char* in, unsigned int len, std::wstring& out);
    void Unicode2Utf8(const wchar_t* in, unsigned int len, std::string& out);
    void Utf82Unicode(const char* in, unsigned int len, std::wstring& out);
    void Ansi2Utf8(const char* in, unsigned int len, std::string& out);
    void Utf82Ansi(const char* in, unsigned int len, std::string& out);

}

#endif
