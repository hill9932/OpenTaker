#include "var.h"

namespace LiangZhu
{
    Variant::Variant()
    {
        m_type = VarType_e::NULL_e;
        bzero(&m_value, sizeof(m_value));
    }

    Variant::Variant(const tchar _c)
    {
        operator= (_c);
    }

    Variant::Variant(const byte _b)
    {
        operator= (_b);
    }

    Variant::Variant(const int16 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const u_int16 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const int32 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const u_int32 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const int64 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const u_int64 _i)
    {
        operator= (_i);
    }

    Variant::Variant(const tchar* _str)
    {
        operator= (_str);
    }

    Variant::Variant(const byte* _buf, const u_int32 _len)
    {
        setValue(_buf, _len);
    }

    Variant::~Variant()
    {
        clear();
    }

    Variant& Variant::operator= (const Variant& _variant)
    {
        if (this == &_variant) return *this;  // check for self assignment

        clear(); 
        memcpy(this, &_variant, sizeof(Variant));

        //
        // these type need deep copy
        //
        if (m_type == STRING_e)    
        {
            m_value.strValue = strdup_t(_variant.m_value.strValue);
        }
        else if (m_type == BLOCK_e)
        {
            m_value.blkValue.buf = new byte[_variant.m_value.blkValue.size];
            memcpy(m_value.blkValue.buf, _variant.m_value.blkValue.buf, _variant.m_value.blkValue.size);
        }

        return *this; // return self reference
    }

    bool Variant::operator== (const Variant& _variant)
    {
        if (m_type != _variant.m_type)  return false;

        if (m_type == STRING_e)
        {
            if (0 != strcmp_t(m_value.strValue, _variant.m_value.strValue))
                return false;
        }
        else if (m_type == BLOCK_e)
        {
            if (m_value.blkValue.size != _variant.m_value.blkValue.size ||
                0 != memcmp(m_value.blkValue.buf, _variant.m_value.blkValue.buf, m_value.blkValue.size))
                return false;
        }
        else if (m_type == CHAR_e || m_type == BYTE_e)
            return m_value.byValue == _variant.m_value.byValue;
        else if (m_type == I16_e || m_type == U16_e)
            return m_value.i16Value == _variant.m_value.i16Value;
        else if (m_type == I32_e || m_type == U32_e)
            return m_value.i32Value == _variant.m_value.i32Value;
        else if (m_type == I64_e || m_type == U64_e)
            return m_value.i64Value == _variant.m_value.i64Value;

        return true;
    }

    bool Variant::operator!= (const Variant& _variant)
    {
        return !operator== (_variant);
    }

    Variant& Variant::operator= (const tchar _c)
    {
        m_type = CHAR_e;
        m_value.cValue = _c;
        return *this;
    }

    Variant& Variant::operator= (const byte _b)
    {
        m_type = BYTE_e;
        m_value.byValue = _b;
        return *this;
    }

    Variant& Variant::operator= (const int16 _i)
    {
        m_type = I16_e;
        m_value.i16Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const u_int16 _i)
    {
        m_type = U16_e;
        m_value.i16Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const int32 _i)
    {
        m_type = I32_e;
        m_value.i32Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const u_int32 _i)
    {
        m_type = U32_e;
        m_value.i32Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const int64 _i)
    {
        m_type = I64_e;
        m_value.i64Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const u_int64 _i)
    {
        m_type = U64_e;
        m_value.i64Value = _i;
        return *this;
    }

    Variant& Variant::operator= (const tchar* _str)
    {
        clear();
        if (!_str)  m_type = NULL_e;
        else
        {
            m_type = STRING_e;
            m_value.strValue = strdup(_str);
        }

        return *this;
    }

    Variant& Variant::setValue(const byte* _buf, const u_int32 _len)
    {
        clear();
        if (!_buf)  m_type = NULL_e;
        else
        {
            m_type = BLOCK_e;
            m_value.blkValue.buf = new byte[_len];
            m_value.blkValue.size = _len;
            memcpy(m_value.blkValue.buf, _buf, _len);
        }

        return *this;
    }
    
    Variant::operator tchar()
    {
        return m_value.cValue;
    }

    Variant::operator byte()
    {
        return m_value.byValue;
    }

    Variant::operator int16()
    {
        return m_value.i16Value;
    }

    Variant::operator int32()
    {
        return m_value.i32Value;
    }

    Variant::operator int64()
    {
        return m_value.i64Value;
    }

    Variant::operator u_int16()
    {
        return m_value.i16Value;
    }

    Variant::operator u_int32()
    {
        return m_value.i32Value;
    }

    Variant::operator u_int64()
    {
        return m_value.i64Value;
    }

    Variant::operator tchar*()
    {
        return m_value.strValue;
    }

    Variant::operator byte*()
    {
        return m_value.blkValue.buf;
    }

    Variant::operator void*()
    {
        return (byte*)*this;
    }

    u_int32 Variant::size()
    {
        u_int32 size = 0;
        if (m_type == STRING_e)
            size = strlen_t(m_value.strValue);
        else if (m_type == BLOCK_e)
            size = m_value.blkValue.size;
        else if (m_type == CHAR_e || m_type == BYTE_e)
            size = 1;
        else if (m_type == I16_e || m_type == U16_e)
            size = 2;
        else if (m_type == I32_e || m_type == U32_e)
            size = 4;
        else if (m_type == I64_e || m_type == U64_e)
            size = 8;

        return size;
    }

    void Variant::clear()
    {
        if (m_type == STRING_e)
            delete[] m_value.strValue;
        else if (m_type == BLOCK_e)
            delete[] m_value.blkValue.buf;

        bzero(&m_value, sizeof(m_value));
    }
}