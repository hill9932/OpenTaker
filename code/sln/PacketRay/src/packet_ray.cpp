#include "packet_ray.h"
#include "app_base.h"


RM_LOG_DEFINE("PacketRay");
AppConf g_conf;

void SetRayLogger(log4cplus::Logger* _logger)
{
    g_logger = _logger;
}
