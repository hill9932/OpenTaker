#ifndef __HILUO_ERR_CODE_INCLUDE_H__
#define __HILUO_ERR_CODE_INCLUDE_H__

namespace LiangZhu
{
    struct ErrorCode
    {
        static void Init()
        {
            if (!errDesc_t)
                errDesc_t = new const char*[STATUS_LAST + 1];

            errDesc_t[STATUS_OK]    = "Success";
            errDesc_t[STATUS_ERROR] = "Unknown error";
            errDesc_t[STATUS_ERROR_PARAMETER] = "Invalid parameter";
            errDesc_t[STATUS_LAST]  = NULL;
        }

        enum
        {
            STATUS_OK = 0,
            STATUS_ERROR,
            STATUS_ERROR_PARAMETER,
            STATUS_LAST
        };

    private:
        static const char** errDesc_t;
    };
}

#endif
