#ifndef __HILUO_VAR_INCLUDE_H__
#define __HILUO_VAR_INCLUDE_H__

#include "common.h"

namespace LiangZhu
{
    struct Variant
    {
        /**
         * Constructor with no arguments.
         * This constructor creates an uninitialized Variant.
         */
        Variant();        
        ~Variant();
        explicit Variant(const tchar _c);
        explicit Variant(const byte _b);
        explicit Variant(const int16 _i);
        explicit Variant(const int32 _i);
        explicit Variant(const int64 _i);
        explicit Variant(const u_int16 _i);
        explicit Variant(const u_int32 _i);
        explicit Variant(const u_int64 _i);
        explicit Variant(const tchar* _str);
        explicit Variant(const byte* _buf, const u_int32 _len);
        Variant(const Variant&  _variant) { *this = _variant; }
        
        bool operator== (const Variant& _variant);
        bool operator!= (const Variant& _variant);

        Variant& operator= (const Variant& _variant);
        Variant& operator= (const tchar _c);
        Variant& operator= (const byte _b);
        Variant& operator= (const int16 _i);
        Variant& operator= (const int32 _i);
        Variant& operator= (const int64 _i);
        Variant& operator= (const u_int16 _i);
        Variant& operator= (const u_int32 _i);
        Variant& operator= (const u_int64 _i);
        Variant& operator= (const tchar* _str);
        Variant& setValue(const byte* _buf, const u_int32 _len);
        
        operator tchar();
        operator byte();
        operator int16();
        operator int32();
        operator int64();
        operator u_int16();
        operator u_int32();
        operator u_int64();
        operator tchar*();
        operator byte*();
        operator void*();

        void clear();
        u_int32 size();
        Variant *clone() const { return new Variant(*this); }

    protected:
        struct block_t
        {
            u_int32 size;
            byte*   buf;
        };

        enum VarType_e
        {
            NULL_e,
            CHAR_e,
            BYTE_e,
            I16_e,
            U16_e,
            I32_e,
            U32_e,
            I64_e,
            U64_e,
            STRING_e,
            BLOCK_e
        } m_type;

        union
        {
            tchar       cValue;
            byte        byValue;
            int16       i16Value;
            int32       i32Value;
            int64       i64Value;
            tchar*      strValue;
            block_t     blkValue;
        } m_value;
    };
}

#endif
