#ifndef __HILUO_VERSION_INCLUDE_H__
#define __HILUO_VERSION_INCLUDE_H__

#define PROJECT_NAME    TEXT("PacketRay")

//
// define the application version
//
#define APP_VERSION_MAJOR	0
#define APP_VERSION_MINOR	1
#define APP_VERSION_NUM     2015
#define APP_VERSION_BUILD	1111
#define APP_VERSION_SLASH   "."

#define APP_CHARS(x)    #x
#define APP_VALUES(x)   APP_CHARS(x)
#define HILUO_PACKAGE_VERSION \
    APP_VALUES(APP_VERSION_MAJOR) APP_VERSION_SLASH \
    APP_VALUES(APP_VERSION_MINOR) APP_VERSION_SLASH \
    APP_VALUES(APP_VERSION_NUM) APP_VERSION_SLASH \
    APP_VALUES(APP_VERSION_BUILD)

#endif
