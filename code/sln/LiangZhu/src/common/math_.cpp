#include "math_.h"
#include "string_.h"

namespace LiangZhu
{
    int GetRandomNumber(int _begin, int _end)
    {
        if (_end < _begin)  return -1;
        else if (_end == _begin)    return _begin;

        srand((unsigned)time(0));
        return rand() % (_end - _begin) + _begin;
    }

    size_t GetAlignedValue(size_t unaligned)
    {
        return((unaligned + SECTOR_ALIGNMENT - 1) & CHUNK_ALIGNMENT_MASK);
    }

    int Log2(u_int64 _value)
    {
        int count = 0;
        u_int64 remain = _value;
        while (remain /= 2)
        {
            ++count;
        }

        return count;
    }

    u_int32 Distance(u_int32 _value1, u_int32 _value2, u_int32 _total)
    {
        u_int32 distance = _value2 > _value1 ? _value2 - _value1 : (_value2 + _total - _value1);
        return distance % _total;
    }

    CStdString ChangeUnit(u_int64 _count, u_int32 _unit, const char* _suffix, int _maxLevel)
    {
        if (!_suffix)   return " ";

        int level = 0;
        const char* unit[] = { " ", "K", "M", "G", "T", "P", "E" };
        int i = 0;
        double dd = (double)_count;
        while (dd >= _unit && i < _maxLevel)
        {
            dd /= _unit;
            ++i;
        }
        if (i >= sizeof(unit) / sizeof(unit[0]))    return " ";

        CStdString v;
        v.Format("%.2f %s%s", dd, unit[i], _suffix);
        return v;
    }

    u_int64 XtoI(const char* _value)
    {
        if (!_value ||
            strlen(_value) <= 2 ||
            _value[0] != '0' ||
            (_value[1] != 'F' && _value[1] != 'f'))
            return 0;

        u_int64 value = 0;
        for (int i = 2; i < strlen(_value); ++i)
        {
            value <<= 4;
            value |= GetXValue(_value[i]);
        }

        return value;
    }

}
