/**
 * @Function:
 *  We use log4cpp as the logger engine.
 *  This file includes necessary headers and declare some macros to help use log4cpp.
 * @Memo:
 *  Created by hill, 4/11/2014
 **/

#ifndef __HILUO_LOG_INCLUDE_H__
#define __HILUO_LOG_INCLUDE_H__

#include "platform.h"

#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
#include "log4cplus/configurator.h"
#include "log4cplus/ndc.h"
#include <boost/shared_ptr.hpp>

extern log4cplus::Logger* g_logger;

namespace LiangZhu
{
    bool InitLog(const tchar* _configure, const ::tchar* _category);

    //
    // The following macros are used to operate log4plus
    //
#define RM_LOG_DEFINE(CategoryName)  log4cplus::Logger* g_logger;// = log4cplus::Logger::getInstance(CategoryName);


#define RM_SET_NDC(str) NDCContextCreator _first_ndc(str);

#if defined(DEBUG) || defined(_DEBUG)
#define RM_LOG_TRACE(str)       {   \
    LOG4CPLUS_TRACE(*g_logger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_TRACE_S(str)        {   \
    LOG4CPLUS_TRACE(*g_logger, " " << str);  \
    }
#define RM_LOG_TRACE_E(mylogger, str)       {   \
    LOG4CPLUS_TRACE(mylogger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_TRACE_ES(mylogger, str)       {   \
    LOG4CPLUS_TRACE(mylogger, " " << str); \
    }

#define RM_LOG_DEBUG(str)       {   \
    LOG4CPLUS_DEBUG(*g_logger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_DEBUG_S(str)        {   \
    LOG4CPLUS_DEBUG(*g_logger, " " << str);  \
    }
#define RM_LOG_DEBUG_E(mylogger, str)       {   \
    LOG4CPLUS_DEBUG(mylogger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_DEBUG_ES(mylogger, str)       {   \
    LOG4CPLUS_DEBUG(mylogger, " " << str); \
    }

#define RM_LOG_INFO(str)        {   \
    LOG4CPLUS_INFO(*g_logger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str);  \
    }
#define RM_LOG_INFO_S(str)        {   \
    LOG4CPLUS_INFO(*g_logger, " " << str);  \
    }
#define RM_LOG_INFO_E(mylogger, str)        {   \
    LOG4CPLUS_INFO(mylogger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str);  \
    }
#define RM_LOG_INFO_ES(mylogger, str)        {   \
    LOG4CPLUS_INFO(mylogger, " " << str);  \
    }

#define RM_LOG_WARNING(str)     {   \
    LOG4CPLUS_WARN(*g_logger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str);  \
    }
#define RM_LOG_WARNING_S(str)        {   \
    LOG4CPLUS_WARN(*g_logger, " " << str);  \
    }
#define RM_LOG_WARNING_E(mylogger, str)        {   \
    LOG4CPLUS_WARN(mylogger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str);  \
    }
#define RM_LOG_WARNING_ES(myloger, str)       {   \
    LOG4CPLUS_WARN(myloger, " " << str); \
    }

#define RM_LOG_ERROR(str)       {   \
    LOG4CPLUS_ERROR(*g_logger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_ERROR_S(str)        {   \
    LOG4CPLUS_ERROR(*g_logger, " " << str);  \
    }
#define RM_LOG_ERROR_E(myloger, str)       {   \
    LOG4CPLUS_ERROR(myloger, " " << GetFileName(__FILE__) << ":" << __FUNCTION__ << "():" << __LINE__ << " " << str); \
    }
#define RM_LOG_ERROR_ES(myloger, str)       {   \
    LOG4CPLUS_ERROR(myloger, " " << str); \
    }

#else
#define RM_LOG_DEBUG(str)       {   \
    LOG4CPLUS_DEBUG(*g_logger, " " << GetFileName(__FILE__) << ":" << __LINE__ << " " << str); \
    }
#define RM_LOG_DEBUG_S(str)        {   \
    LOG4CPLUS_DEBUG(*g_logger,  " " << str);  \
    }
#define RM_LOG_DEBUG_E(mylogger, str)       {   \
    LOG4CPLUS_DEBUG(mylogger, " " << GetFileName(__FILE__) << ":" << __LINE__ << " " << str); \
    }
#define RM_LOG_DEBUG_ES(mylogger, str)       {   \
    LOG4CPLUS_DEBUG(mylogger, " " << str); \
    }

#define RM_LOG_TRACE(str)       {   \
    LOG4CPLUS_TRACE(*g_logger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str); \
    }
#define RM_LOG_TRACE_S(str)        {   \
    LOG4CPLUS_TRACE(*g_logger, " " << str);  \
    }
#define RM_LOG_TRACE_E(mylogger, str)       {   \
    LOG4CPLUS_TRACE(mylogger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str); \
    }
#define RM_LOG_TRACE_ES(mylogger, str)       {   \
    LOG4CPLUS_TRACE(mylogger, " " << str); \
    }

#define RM_LOG_INFO(str)        {   \
    LOG4CPLUS_INFO(*g_logger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str);  \
    }
#define RM_LOG_INFO_S(str)        {   \
    LOG4CPLUS_INFO(*g_logger, " " << str);  \
    }
#define RM_LOG_INFO_E(mylogger, str)        {   \
    LOG4CPLUS_INFO(mylogger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str);  \
    }
#define RM_LOG_INFO_ES(mylogger, str)        {   \
    LOG4CPLUS_INFO(mylogger, " " << str);  \
    }

#define RM_LOG_WARNING(str)     {   \
    LOG4CPLUS_WARN(*g_logger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str);  \
    }
#define RM_LOG_WARNING_S(str)        {   \
    LOG4CPLUS_WARN(*g_logger,  " " << str);  \
    }
#define RM_LOG_WARNING_E(mylogger, str)        {   \
    LOG4CPLUS_WARN(mylogger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str);  \
    }
#define RM_LOG_WARNING_ES(myloger, str)       {   \
    LOG4CPLUS_WARN(myloger, " " << str); \
    }

#define RM_LOG_ERROR(str)       {   \
    LOG4CPLUS_ERROR(*g_logger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str); \
    }
#define RM_LOG_ERROR_S(str)        {   \
    LOG4CPLUS_ERROR(*g_logger, " " << str);  \
    }
#define RM_LOG_ERROR_E(myloger, str)       {   \
    LOG4CPLUS_ERROR(myloger, " " << GetFileName(__FILE__) << ": " << __LINE__ << " " << str); \
    }
#define RM_LOG_ERROR_ES(myloger, str)       {   \
    LOG4CPLUS_ERROR(myloger, " " << str); \
    }
#endif

}

#endif
