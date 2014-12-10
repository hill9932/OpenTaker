#ifndef __HILUO_DEVICE_INCLUDE_H__
#define __HILUO_DEVICE_INCLUDE_H__

#include "common.h"

enum PortStatus_e
{
    LINK_UNKNOWN = 0,
    LINK_OFF,
    LINK_ON,
    LINK_ERROR
};

struct PortStat_t
{
    PortStatus_e    status;
    CStdString      mac;
    CStdString      id;         // maybe the pci address
};

enum DeviceType_e
{
    DEVICE_TYPE_UNKNOW,
    DEVICE_TYPE_FILE_PCAP,
    DEVICE_TYPE_LIBPCAP,
    DEVICE_TYPE_INTEL,
    DEVICE_TYPE_NAPATECH,
    DEVICE_TYPE_ACCOLADE,
    DEVICE_TYPE_VIRTUAL,
    DEVICE_TYPE_MEMORY
};

struct DeviceST
{
    DeviceType_e type;
    CStdString  name;		    // name to hand to "pcap_open_live()"
    CStdString  desc;	        // textual description of interface, or NULL

    u_int64     productID;
    u_int32     linkSpeed;      // speed of one port
    u_int32     portCount;
    u_int32     portIndex;

    u_int32     myDevIndex;     // device index with the same kind device
    u_int32     myPortIndex;    // port index with the same kind device
    PortStat_t  portStat[4];    // at most 4 ports every device

    DeviceST()
    {
        portCount = 1;
        portIndex = 0;
        myPortIndex = 0;
        myDevIndex = 0;
        linkSpeed = 0;

        for (int i = 0; i < 4; ++i)
            portStat[i].status = LINK_UNKNOWN;
    }
};

#endif
