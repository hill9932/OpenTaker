#ifndef __HILUO_VAR_INCLUDE_H__
#define __HILUO_VAR_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    struct VarType_e
    {
        enum {
            CHAR_e,
            BYTE_e,
            WORD_e,
            DWORD_e,
            QWORD_e,
            STRING_e
        };
    };


    struct string_t
    {
        u_int32 len;
        char*   buf;
    };


    struct Var
    {
        VarType_e   type;

        union
        {
            char        cValue;
            byte        byValue;
            int16       i16Value;
            u_int32     i32Value;
            u_int64     i64Value;
            string_t    sValue;
        } value;
    };
}


#endif
