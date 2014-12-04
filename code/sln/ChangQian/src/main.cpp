#include "version.h"
#include "common.h"

namespace ChangQian
{
    CStdString GetLibVersion()
    {
        CStdString version = PROJECT_NAME;
#if defined(DEBUG) || defined(_DEBUG)
        version += " DEBUG ";
#else
        version += "RELEASE ";
#endif
        version += HILUO_PACKAGE_VERSION;
        return version;
    }
}
