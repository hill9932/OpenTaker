#ifndef __HILUO_PACKET_RAY_INCLUDE_H__
#define __HILUO_PACKET_RAY_INCLUDE_H__

#include "common.h"


#ifdef PR_EXPORTS

#ifdef WIN32
#define PR_API __declspec(dllexport)
#elif defined(LINUX)
#define PR_API
#endif

#else

#ifdef WIN32
#define PR_API __declspec(dllimport)
#elif defined(LINUX) 
#define PR_API
#endif

#endif

PR_API
void SetRayLogger(log4cplus::Logger* _logger);

#endif
