#include "common.h"
#include "system_.h"
#include "global.h"
#include "version.h"
#include "pcap_capture.h"
#include "virtual_capture.h"
#include "capture_store.h"

RM_LOG_DEFINE(PROJECT_NAME);


void ShowConfig()
{
    CStdString version = "RELEASE  ";
#if defined(DEBUG) || defined(_DEBUG)
    version = "DEBUG  ";
#endif
    version += HILUO_PACKAGE_VERSION;

    RM_LOG_INFO_S("Success to initialize the environment (" << version << ", " << \
                   GetProgramBits() << " bit, pid: " << GetCurrentPID() << ").");
    RM_LOG_INFO_S("Config file = " << g_env->m_config.configFile);
 
}

int main(int _argc, char** _argv)
{
    if (!SetupEnviroment(_argc, _argv, PROJECT_NAME))
        return -1;
    ShowConfig();

    CPcapCapture::scanLocalNICs();
    CVirtualCapture::scanLocalNICs();

    //
    // enumerate the NICs on the system and choose one to open
    //
    int n = 0;
    CBlockCaptureImpl *manager = CBlockCaptureImpl::GetInstance();
    if ((n = manager->showLocalNICs()) <= 0)  return 0;

    cout << "Please select a NIC to capture: ";
    int index = 0;
    cin >> index;

    if (index < 1 || index > n)
    {
        RM_LOG_ERROR("The selected NIC is invalid.");
        return -1;
    }
    --index;

    //
    // open the device and prepare to start capture
    //
    vector<DeviceST> NICs;
    manager->getLocalNICs(NICs);
    CNetCapture * devCapture = NULL;
    switch (NICs[index].type)
    {
    case DEVICE_TYPE_LIBPCAP:
        devCapture = new CPcapCapture;
        break;
    case DEVICE_TYPE_VIRTUAL:
        devCapture = new CVirtualCapture;
        break;
    default:
        break;
    }
    assert(devCapture);
    if (!devCapture)   return -1;

    if (0 != devCapture->openDevice(index))
    {
        RM_LOG_ERROR("Fail to open the device: " << index + 1);
        return -1;
    }

    CaptureConfig_t _config;
    if (0 == devCapture->startCapture(_config))
    {
        CBlockCaptureImpl::GetInstance()->m_captureState = CAPTURE_STARTING;
    }

    while (g_env->enable())
        SleepSec(1);

    RM_LOG_INFO("I'm going to exit.");
    return 0;
}