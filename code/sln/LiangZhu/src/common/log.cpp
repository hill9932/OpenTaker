#include "log.h"
#include "common.h"
#include "file_system.h"
#include "log4cplus/helpers/loglog.h"

bool InitLog(const ::tchar* _configure, const ::tchar* _category)
{
	try 
	{
        if (g_logger)
            return true;

        if (!CanFileAccess(_configure))
        {
            cerr << "Log configure file '" << _configure << "' is invalid." << endl;
            return false;
        }

        if (!CreateAllDir("./log")) // the dir to keep log files
        {
            cerr << "Fail to create the log directory: ./log" << endl;
            return false;
        }

        log4cplus::initialize();
        log4cplus::PropertyConfigurator::doConfigure(_configure);
        log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(false);
        g_logger = new log4cplus::Logger(log4cplus::Logger::getInstance(_category));

        //log4cplus::ConfigureAndWatchThread(_configure, 60 * 1000);
#ifdef WIN32
        setlocale(LC_ALL, "chs");
#endif
	}
	catch (...)  
	{
		std::cerr << "ERROR: Fail to init log4plus.";// << e.what() << std::endl;
		return false;
	}

	return true;
}
