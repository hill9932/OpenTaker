#include "code_convert.h"

namespace LiangZhu
{
#ifdef WIN32
    void Unicode2Utf8(const wchar_t* _in, unsigned int _len, std::string& _out)
    {
        size_t out_len = _len * 3 + 1;
        AutoFree<char, _RELEASE_CHAR_> buf(new char[out_len], DeleteArray);
        if (NULL == buf.Get())
            return;

        memset(buf, 0, out_len);

        out_len = ::WideCharToMultiByte(CP_UTF8, 0, _in, _len, buf, _len * 3, NULL, NULL);
        _out.assign(buf, out_len);

        return;
    }

    void Utf82Unicode(const char* _in, unsigned int _len, std::wstring& _out)
    {
        AutoFree<wchar_t, _RELEASE_WCHAR_> buf(new wchar_t[_len + 1], DeleteArray);
        if (NULL == buf.Get())
            return;

        size_t out_len = (_len + 1) * sizeof(wchar_t);
        memset(buf, 0, (_len + 1) * sizeof(wchar_t));

        out_len = ::MultiByteToWideChar(CP_UTF8, 0, _in, _len, buf, _len * sizeof(wchar_t));
        _out.assign(buf, out_len);
    }

    void Unicode2Ansi(const wchar_t* _in, unsigned int _len, std::string& _out)
    {
        int bufferlen = (int)::WideCharToMultiByte(CP_ACP, 0, _in, (int)_len, NULL, 0, NULL, NULL);
        AutoFree<char, _RELEASE_CHAR_> buf(new char[bufferlen + 4], DeleteArray);
        if (NULL == buf.Get())
            return;

        int out_len = ::WideCharToMultiByte(CP_ACP, 0, _in, (int)_len, buf, bufferlen + 2, NULL, NULL);
        buf[bufferlen] = '\0';
        _out.assign(buf, out_len);
    }

    void Ansi2Unicode(const char* _in, unsigned int _len, std::wstring& _out)
    {
        int wbufferlen = (int)::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, _in, (int)_len, NULL, 0);
        AutoFree<wchar_t, _RELEASE_WCHAR_> buf(new wchar_t[wbufferlen + 4], DeleteArray);
        if (NULL == buf.Get())
            return;

        wbufferlen = (int)::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, _in, (int)_len, buf, wbufferlen + 2);
        buf[wbufferlen] = '\0';
        _out.assign(buf, wbufferlen);
    }


    void Utf82Ansi(const char* _in, unsigned int _len, std::string& _out)
    {
        std::wstring wname;
        Utf82Unicode(_in, _len, wname);
        Unicode2Ansi(wname.c_str(), wname.size(), _out);
    }

    void Ansi2Utf8(const char* _in, unsigned int _len, std::string& _out)
    {
        wstring wname;
        Ansi2Unicode(_in, _len, wname);
        Unicode2Utf8(wname.c_str(), wname.size(), _out);
    }


#else

#include <iconv.h>


    int ConvertCode(const char *_fromCharSet, const char *_toCharSet, char *_inBuf, size_t _inLen, char *_outBuf, size_t _outLen)
    {
        iconv_t cd;
        int rc = 0;
        char *pin  = _inBuf;
        char *pout = _outBuf;
        size_t outLen = _outLen;

        cd = iconv_open(_toCharSet, _fromCharSet);
        if (cd == 0) return -1;

        memset(_outBuf, 0, _outLen);
        if ((rc = iconv(cd, &pin, &_inLen, &pout, &outLen)) == -1) return -1;
        iconv_close(cd);

        return _outLen - outLen;
    }

    void Unicode2Ansi(const wchar_t* _in, unsigned int _len, std::string& _out)
    {
        int sz = 0;
        AutoFree<char, _RELEASE_CHAR_> buf(new char[_len+1], DeleteArray);
        if ((sz = ConvertCode("UNICODE", "ASCII", (char*)_in, _len * sizeof(wchar_t), buf, _len + 1)) > 0)
            _out.assign(buf, sz);
    }

    void Ansi2Unicode(const char* _in, unsigned int _len, std::wstring& _out)
    {
        int sz = 0;
        AutoFree<wchar_t, _RELEASE_WCHAR_> buf(new wchar_t[_len+1], DeleteArray);
        if ((sz = ConvertCode("ASCII", "UNICODE", (char*)_in, _len, (char*)buf.Get(), (_len + 1) * sizeof(wchar_t))) > 0)
            _out.assign(buf, sz);
    }

    void Unicode2Utf8(const wchar_t* _in, unsigned int _len, std::string& _out)
    {
        int sz = 0;
        AutoFree<char, _RELEASE_CHAR_> buf(new char[_len+1], DeleteArray);
        if ((sz = ConvertCode("UNICODE", "UTF-8", (char*)_in, _len * sizeof(wchar_t), buf, _len + 1)) > 0)
            _out.assign(buf, sz);
    }

    void Utf82Unicode(const char* _in, unsigned int _len, std::wstring& _out)
    {
        int sz = 0;
        AutoFree<wchar_t, _RELEASE_WCHAR_> buf(new wchar_t[_len+1], DeleteArray);
        if ((sz = ConvertCode("UTF-8", "UNICODE", (char*)_in, _len, (char*)buf.Get(), (_len + 1) * sizeof(wchar_t))) > 0)
            _out.assign(buf, sz);
    }

    void Ansi2Utf8(const char* _in, unsigned int _len, std::string& _out)
    {
        int sz = 0;
        AutoFree<char, _RELEASE_CHAR_> buf(new char[_len*2], DeleteArray);
        if ((sz = ConvertCode("ASCII", "UTF-8", (char*)_in, _len, buf, _len * 2)) > 0)
            _out.assign(buf, sz);
    }

    void Utf82Ansi(const char* _in, unsigned int _len, std::string& _out)
    {
        int sz = 0;
        AutoFree<char, _RELEASE_CHAR_> buf(new char[_len*2], DeleteArray);
        if ((sz = ConvertCode("UTF-8", "ASCII", (char*)_in, _len, buf, _len * 2)) > 0)
            _out.assign(buf, sz);
    }

#endif

}
