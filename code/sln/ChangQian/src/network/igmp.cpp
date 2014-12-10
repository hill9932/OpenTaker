#include "common.h"

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

enum MULTICAST_MODE_TYPE
{
    MCAST_INCLUDE_ = 0,
    MCAST_EXCLUDE_
};

#endif


/**
 * This routine joins the multicast group for the given sources. If the mode
 * is include then IP_ADD_SOURCE_MEMBERSHIP is used on each source. 
 * If the mode is exclude then we join the group like
 * normal (using IP_ADD_MEMBERSHIP) and then we drop each of the source IPs
 * given using IP_DROP_MEMBERSHIP. 
 *
 * @param sd,   the socket to set
 * @param _mode, multicast mode whether include or exclude
 * @param _grpaddr, the multicast group address
 * @param _ifaddr,  the local interface address which will join the multicast group
 * @param _srcaddr_count, the count of source addresses
 * @param _srcaddrs, the source addresses as SSM
 *
 * @return 0, success
 **/
int JoinSourceGroup(int sd, 
                      MULTICAST_MODE_TYPE _mode, 
                      u_int32   _grpaddr, 
                      u_int32   _ifaddr,
                      int       _srcaddr_count,
                      u_int32*  _srcaddrs )
{
    if (!_srcaddrs) return -1;

    in_addr grp_addr, if_addr, src_addr;
    grp_addr.s_addr = _grpaddr;
    if_addr.s_addr  = _ifaddr;
    string sgrp = inet_ntoa(grp_addr);
    string sif  = inet_ntoa(if_addr);

    if (_mode == MCAST_INCLUDE)
    {
        // If the mode is include, call IP_ADD_SOURCE_MEMBERSHIP for each source.
        // In this sample we only add sources but in a more dynamic environment
        // you could remove sources that were previously added by calling IP_DROP_SOURCE_MEMBERSHIP.
        for (int i=0; i < _srcaddr_count; i++)
        {
            src_addr.s_addr = _srcaddrs[i];

            struct ip_mreq_source imr = {0}; 
            imr.imr_multiaddr.s_addr  = _grpaddr;
            imr.imr_interface.s_addr  = _ifaddr;
            imr.imr_sourceaddr.s_addr = _srcaddrs[i];
 
            // Add the source membership
            int rc = setsockopt(sd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&imr, sizeof(imr));  
            if (0 != rc)
            {
                in_addr inaddr;
                inaddr.s_addr = _grpaddr;
                printf("setsockopt(): IP_ADD_SOURCE_MEMBERSHIP failed with error code (%u)\n", net_errno);
            }
            else
            {               
                printf("ADD SOURCE: %s for GROUP: %s on INTERFACE: %s\n", inet_ntoa(src_addr), sgrp.c_str(), sif.c_str());   
            }
        }
    }
    else if (_mode == MCAST_EXCLUDE)
    {
        // For exclude mode, we first need to join the multicast group with
        // IP_ADD_MEMBERSHIP (which sets the mode to EXCLUDE NONE). After which
        // we drop those sources that we want to block from membership via the
        // IP_BLOCK_SOURCE. This sample only drops sources but does not allow
        // them back into the group. To add back a source that was previously
        // dropped, you would call IP_UNBLOCK_SOURCE.
        struct ip_mreq_source imr = {0}; 
        imr.imr_multiaddr.s_addr  = _grpaddr;
        imr.imr_interface.s_addr  = _ifaddr;

        // First join the group (state will be EXCLUDE NONE)
        int rc = setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&imr, sizeof(imr));
        if (0 != rc)
        {
            printf("setsockopt(): IP_ADD_MEMBERSHIP failed with error code (%u)\n", net_errno);
            return SOCKET_ERROR;
        }
        else
        {
            printf("JOINED GROUP: %s on INTERFACE: %s\n", sgrp.c_str(), sif.c_str());   
        }

        // Now drop each source from the group
        for (int i=0; i < _srcaddr_count; i++)
        {
            struct ip_mreq_source imr; 
            imr.imr_multiaddr.s_addr  = _grpaddr;
            imr.imr_interface.s_addr  = _ifaddr;
            imr.imr_sourceaddr.s_addr = _srcaddrs[i];

            int rc = setsockopt(sd, IPPROTO_IP, IP_BLOCK_SOURCE, (char *)&imr, sizeof(imr));
            if (0 != rc)
            {
                printf("setsockopt(): IP_BLOCK_SOURCE failed with error code (%u)\n", net_errno);
            }
            else
            {
                src_addr.s_addr = _srcaddrs[i];
                printf("   DROPPED SOURCE: %s\n", inet_ntoa(src_addr));
            }
        }
    }

    return 0;
}


int LeaveSourceGroup(int sd, u_int32 _grpaddr, u_int32 _srcaddr, u_int32 _iaddr)
{
    struct ip_mreq_source imr;

    imr.imr_multiaddr.s_addr  = _grpaddr;
    imr.imr_sourceaddr.s_addr = _srcaddr;
    imr.imr_interface.s_addr  = _iaddr;
    return setsockopt(sd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, (char *) &imr, sizeof(imr));
}

void GetMulticastState(int sd, struct addrinfo *group, struct addrinfo *iface)
{
    struct ip_msfilter *filter=NULL;
    char                buf[15000];
    int                 buflen=15000, rc, i;
 
    filter = (struct ip_msfilter *)buf;
 
    filter->imsf_multiaddr = ((sockaddr_in *)group->ai_addr)->sin_addr;
    filter->imsf_interface = ((sockaddr_in *)iface->ai_addr)->sin_addr;
 
#ifdef WIN32
    rc = WSAIoctl(sd, SIO_GET_MULTICAST_FILTER, buf, buflen, buf, buflen, (LPDWORD)&buflen, NULL, NULL);
#endif

    if (rc == SOCKET_ERROR)
    {
        fprintf(stderr, "GetMulticastState: WSAIoctl() failed with error code %d\n", net_errno);
        return;
    }
 
    printf("imsf_multiaddr = %s\n", inet_ntoa(filter->imsf_multiaddr));
    printf("imsf_interface = %s\n", inet_ntoa(filter->imsf_interface));
    printf("imsf_fmode     = %s\n", (filter->imsf_fmode == MCAST_INCLUDE ? "MCAST_INCLUDE" : "MCAST_EXCLUDE"));
    printf("imsf_numsrc    = %d\n", filter->imsf_numsrc);
    
    for(i=0; i < (int)filter->imsf_numsrc ;i++)
    {
        printf("imsf_slist[%d]  = %s\n", i, inet_ntoa(filter->imsf_slist[i]));
    }
    return;
}
 
// Function: SetSendInterface
// Description:  Set the send interface for the socket.
int SetSendInterface(int sd, struct addrinfo *iface)
{
    char *optval=NULL;
    int   optlevel, option, optlen, rc;
 
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_IF;
    optval   = (char *) &((sockaddr_in *)iface->ai_addr)->sin_addr.s_addr;
    optlen   = sizeof(((sockaddr_in *)iface->ai_addr)->sin_addr.s_addr);
 
    rc = setsockopt(sd, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
        printf("setsockopt() failed with error code %d\n", net_errno);
    }
    else
    {
        printf("Set sending interface to: ");
        //PrintAddress(iface->ai_addr, iface->ai_addrlen);
        printf("\n");
    }

    return rc;
}
 
// Function: SetMulticastTtl
// Description: This routine sets the multicast TTL on the socket.
int SetMulticastTTL(int sd, int af, int ttl)
{
    char *optval=NULL;
    int   optlevel, option, optlen, rc;
 
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_TTL;
    optval   = (char *) &ttl;
    optlen   = sizeof(ttl);
 
    rc = setsockopt(sd, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
        fprintf(stderr, "SetMulticastTtl: setsockopt() failed with error code %d\n", net_errno);
    }
    else
    {
        printf("Set multicast ttl to: %d\n", ttl);
    }
    return rc;
}
 
// Function: SetMulticastLoopBack
// Description:
//    This function enabled or disables multicast loopback. If loopback is enabled
//    (and the socket is a member of the destination multicast group) then the
//    data will be placed in the receive queue for the socket such that if a
//    receive is posted on the socket its own data will be read. For this sample
//    it doesn't really matter as if invoked as the sender, no data is read.
int SetMulticastLoopBack(int sd, int af, int loopval)
{
    char *optval=NULL;
    int   optlevel, option, optlen, rc;
 
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_LOOP;
    optval   = (char *) &loopval;
    optlen   = sizeof(loopval);
 
    rc = setsockopt(sd, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
        fprintf(stderr, "SetMulticastLoopBack: setsockopt() failed with error code %d\n", net_errno);
    }
    else
    {
        printf("Setting multicast loopback to: %d\n", loopval);
    }

    return rc;
}

