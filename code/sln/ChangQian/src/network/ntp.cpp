#include "ntp.h"


#define NTP_PCK_LEN 48
#define LI          0
#define VN          3
#define MODE        3
#define STRATUM     0
#define POLL        4
#define PREC       -6

#define JAN_1970        0x83aa7e80 /* second between 1900 - 1970 */
#define NTPFRAC(x)      (4294 * (x) + ((1981 * (x)) >> 11))
#define USEC(x)         (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))


/**
* @Function: Construct NTP packet
**/
int ConstructPacket(int _version, char* _packet)
{
    if (!_packet || _version < 1 || _version > 4)
        return -1;

    char* p = _packet;
    u_int32 tmp_wrd;
    memset(p, 0, NTP_PCK_LEN);

    //
    // set the packet header, 16 B
    //
    tmp_wrd = htonl((LI << 30) |
        (_version << 27) |
        (MODE << 24) |
        (STRATUM << 16) |
        (POLL << 8) |
        (PREC & 0xff));
    memcpy(p, &tmp_wrd, sizeof(tmp_wrd));
    p += sizeof(u_int32);

    tmp_wrd = htonl(1 << 16);
    memcpy(p, &tmp_wrd, sizeof(tmp_wrd));
    p += sizeof(u_int32);

    memcpy(p, &tmp_wrd, sizeof(tmp_wrd));
    p += sizeof(u_int32);

    //
    // set Timestamp
    //
    time_t timer;
    time(&timer);
    tmp_wrd = htonl(JAN_1970 + (long)timer);
    memcpy(&_packet[40], &tmp_wrd, sizeof(tmp_wrd));

    tmp_wrd = htonl((long)NTPFRAC(timer));
    memcpy(&_packet[44], &tmp_wrd, sizeof(tmp_wrd));
    return NTP_PCK_LEN;
}

/* get NTP time */
int GetNTPTime(SOCKET _socket,
    sockaddr *addr,
    int _version,
    int _timeout,
    int _sample,
struct NtpPacket* _ntpTime)
{
    fd_set pending_data;
    struct timeval block_time;
    char data[NTP_PCK_LEN * 8] = { 0 };
    int packet_len, count = 0, result;
    int sockLen = sizeof(sockaddr);

    if (!(packet_len = ConstructPacket(_version, data)))
    {
        return -1;
    }

    result = sendto(_socket,
        data,
        packet_len,
        0,
        addr,
        sockLen);
    ON_ERROR_LOG_LAST_ERROR_AND_DO(result, <, 0, return -1);

    /* select() and wait for 1s */
    FD_ZERO(&pending_data);
    FD_SET(_socket, &pending_data);

    block_time.tv_sec = _timeout;
    block_time.tv_usec = 0;
    if (select(_socket + 1, &pending_data, NULL, NULL, &block_time) > 0)
    {
        count = recvfrom(_socket,
            data,
            NTP_PCK_LEN * 8,
            0,
            addr,
            (socklen_t*)&sockLen);

        ON_ERROR_LOG_LAST_ERROR_AND_DO(count, <, 0, return -1);

        _ntpTime->leap_ver_mode = ntohl(data[0]);
        _ntpTime->startum = ntohl(data[1]);
        _ntpTime->poll = ntohl(data[2]);
        _ntpTime->precision = ntohl(data[3]);
        _ntpTime->root_delay = ntohl(*(int*)&(data[4]));
        _ntpTime->root_dispersion = ntohl(*(int*)&(data[8]));
        _ntpTime->reference_identifier = ntohl(*(int*)&(data[12]));
        _ntpTime->reference_timestamp.coarse = ntohl(*(int*)&(data[16]));
        _ntpTime->reference_timestamp.fine = ntohl(*(int*)&(data[20]));
        _ntpTime->originage_timestamp.coarse = ntohl(*(int*)&(data[24]));
        _ntpTime->originage_timestamp.fine = ntohl(*(int*)&(data[28]));
        _ntpTime->receive_timestamp.coarse = ntohl(*(int*)&(data[32]));
        _ntpTime->receive_timestamp.fine = ntohl(*(int*)&(data[36]));
        _ntpTime->transmit_timestamp.coarse = ntohl(*(int*)&(data[40]));
        _ntpTime->transmit_timestamp.fine = ntohl(*(int*)&(data[44]));

        return 0;

    } /* end of if select */

    return -1;

}


int SetLocalTimeFromNTP(struct NtpPacket * pnew_time_packet)
{
    struct timeval tv;

    tv.tv_sec = pnew_time_packet->transmit_timestamp.coarse - JAN_1970;
    tv.tv_usec = USEC(pnew_time_packet->transmit_timestamp.fine);

    return settimeofday(&tv, NULL);
}