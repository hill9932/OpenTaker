#include "string_.h"

int StrSplit(const tchar* _src, const tchar* _delim, vector<CStdString>& _vec)
{
    if (!_src)  return -1;
    return StrSplit(CStdString(_src), _delim, _vec);
}

int StrSplit(const CStdString& _src, const tchar* _delim, vector<CStdString>& _vec)
{
    std::size_t pos = 0;
    std::size_t found = _src.find_first_of(_delim, pos);
    while (found != std::string::npos)
    {
        _vec.push_back(_src.substr(pos, found - pos));
        pos = found + 1;
        found = _src.find_first_of(_delim, pos);
    }

    _vec.push_back(_src.substr(pos));
    return _vec.size();
}

int StrSplit_s(const tchar* _src, const tchar* _delim, vector<CStdString>& _vec)
{
    return StrSplit_s(CStdString(_src), _delim, _vec);
}

int StrSplit_s(const CStdString& _src, const tchar* _delim, vector<CStdString>& _vec)
{
    std::size_t pos = 0;
    std::size_t found = _src.find(_delim, pos);
    while (found != std::string::npos)
    {
        _vec.push_back(_src.substr(pos, found - pos));
        pos = found + 1;
        found = _src.find(_delim, pos);
    }

    _vec.push_back(_src.substr(pos));
    return _vec.size();
}

tchar* StrTrim(tchar* _src)
{
    if (!_src)  return NULL;

    while (isspace_t(_src[0])) _src++;
    if(_src[0] == 0) return _src;

    tchar* end = &_src[strlen_t(_src) - 1];
    while (end > _src && isspace_t(end[0])) end--;
    end[1] = 0;

    return _src;
}

char* Str2Argv(const tchar* _cmd, tchar** _argv, int& _argc)
{
    if (!_cmd || _argc < 1)   return NULL;
    tchar* buf = strdup_t(_cmd);
    tchar* tmp = buf;
    buf = StrTrim(buf);

    int  argc = 0;
    bool hasQuote = false;

    _argv[argc++] = buf;
    if (buf[0] == '\'' || buf[0] == '\"')
    {
        hasQuote = true;
        ++buf;
    }

    //
    // get the program path surround by "'" or '"'
    //
    if (hasQuote)
    {
        while (buf && *buf && *buf != '\'' && *buf != '\"') ++buf;
        *++buf = 0;

        ++buf;
        if (buf && *buf)
            _argv[argc++] = buf;
    }

    if (argc >= _argc) 
    {
        free(tmp); 
        return NULL;
    }

    if (!buf || !*buf)     
    {
        free(tmp);
        return NULL;
    }

    //
    // get the arguments split by " "
    //
    while (buf && *buf)
    {
        if (*buf == ' ') 
        {
            *buf = 0;
            _argv[argc++] = ++buf;
            if (argc >= _argc)
                break;
        }
        else
            ++buf;
    }

    _argc = argc;
    return tmp;
}

void Bin2Str(const byte* _p, char* _to, size_t _len) 
{
    static const char *hex = "0123456789abcdef";

    for (; _len--; _p++) 
    {
        *_to++ = hex[_p[0] >> 4];
        *_to++ = hex[_p[0] & 0x0f];
    }
    *_to = '\0';
}

int GetXValue(char _x)
{
    switch (_x)
    {
    case 'F':
    case 'f':
        return 15;
    case 'E':
    case 'e':
        return 14;
    case 'D':
    case 'd':
        return 13;
    case 'C':
    case 'c':
        return 12;
    case 'B':
    case 'b':
        return 11;
    case 'A':
    case 'a':
        return 10;
    default:
        return _x - '0';
    }

    return 0;
}

byte AtoX(const char* _xcode)
{
    if (!_xcode)    return 0;
    if (strlen(_xcode) > 2) return 0;

    byte value = 0;
    for (size_t i = 0; i < strlen(_xcode); ++i)
    {
        value <<= 4;
        value += GetXValue(_xcode[i]);
    }
    return value;
}

u_int32 HashString(const tchar* _name)
{
    u_int32 value = 0;
    int i = 3;

    while (_name && *_name)
    {
        value += *_name << 8 * i--;
        ++_name;
        if (i == 0) i = 3;
    }

    return value;
}