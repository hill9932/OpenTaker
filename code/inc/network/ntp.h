#ifndef __VP_NTP_INCLUDE_H__
#define __VP_NTP_INCLUDE_H__

#include "common.h"

#define NTP_PORT            123              // NTP port
  

struct NtpTime_t
{
    unsigned int coarse;
    unsigned int fine;
};

struct NtpPacket
{
    unsigned char leap_ver_mode;
    unsigned char startum;
    char poll;
    char precision;
    int root_delay;
    int root_dispersion;
    int reference_identifier;

    NtpTime_t reference_timestamp;
    NtpTime_t originage_timestamp;
    NtpTime_t receive_timestamp;
    NtpTime_t transmit_timestamp;
};


int GetNTPTime( SOCKET _socket, 
                sockaddr *addr,
                int _version,
                int _timeout,
                int _sample,
                struct NtpPacket* _ntpTime);

int SetLocalTimeFromNTP(struct NtpPacket * pnew_time_packet);

#endif
