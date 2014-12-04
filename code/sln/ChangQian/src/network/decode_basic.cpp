#include "decode_basic.h"
#include "network/protocol_header_l4.h"


void ProcessPacketL4_(u_int16 _l4_packet_len, u_int8 _l4_proto, const u_int8 *_l4h,
                      u_int16 _rawSize, PacketMeta_t* _metaInfo);

int ProcessPacket2(byte* _data, PacketMeta_t* _metaInfo)
{
    if (!_metaInfo || !_data) return -1;

    // clear meta info
    _metaInfo->basicAttr.srcIP   = _metaInfo->basicAttr.dstIP = 0;
    _metaInfo->basicAttr.srcPort = _metaInfo->basicAttr.dstPort = 0;

    u_int16 ip_offset = sizeof(struct ether_header_t);
    const byte* packet = _data;
    u_int16 type = ((ether_header_t*)packet)->type;

    while (type != HO_ETH_IP4 && type != HO_ETH_IP6)
    {
        if (type == HO_ETH_VLAN)
        {
            type = *(u_int16*)(packet + ip_offset + 2);
            ip_offset += 4;
        }
        else if (type == HO_ETH_MPLS)
        {
            type = HO_ETH_IP4;
            do // go through the labels
            {
                ip_offset += 4;
            } while ((packet[ip_offset + 3] & 1) != 0); // check bottom-of-stack indicator
            break;
        }
        else if (type == HO_ETH_PPPoE)
        {
            type = HO_ETH_IP4;
            ip_offset += 8;
            break;
        }
        else
        {
            break;
        }
    }

    //
    // just work on Ethernet packets that contain IP
    //
    switch (type)
    {
    case HO_ETH_IP4:
        if (_metaInfo->basicAttr.pktLen >= ip_offset + (u_int32)20)
        {
            struct ipv4_header_t *iph = (struct ipv4_header_t *)&packet[ip_offset];
            if (iph->getVersion() != 4) // This is not IPv4
                return -1;

            if (iph->protocol == IPPROTO_UDP && !iph->isFragment()) // process GTP-U
            {
                u_short ip_len = iph->getHeaderLen();
                struct udp_header_t *udp = (struct udp_header_t *)&packet[ip_offset + ip_len];

                if ((udp->srcPort == NT_GTP_U_V1_PORT) || (udp->dstPort == NT_GTP_U_V1_PORT))
                {
                    u_int offset = (u_int)(ip_offset + ip_len + sizeof(struct udp_header_t));
                    u_int8 flags = packet[offset];
                    u_int8 message_type = packet[offset + 1];

                    if ((((flags & 0xE0) >> 5) == 1 /* GTPv1 */) && (message_type == 0xFF /* T-PDU */))
                    {
                        ip_offset = ip_offset + ip_len + sizeof(struct udp_header_t) + 8 /* GTPv1 header len */;

                        if (flags & 0x04) ip_offset += 1; /* next_ext_header is present */
                        if (flags & 0x02) ip_offset += 4; /* sequence_number is present (it also includes next_ext_header and pdu_number) */
                        if (flags & 0x01) ip_offset += 1; /* pdu_number is present */

                        iph = (struct ipv4_header_t *) &packet[ip_offset];
                        if (iph->getVersion() != 4) // TODO: Add IPv6 support
                            return -1;
                        if (_metaInfo->basicAttr.pktLen - ip_offset < 20)
                            return -1;
                    }
                }
            }

            _metaInfo->basicAttr.version = 0;
            _metaInfo->basicAttr.srcIP = ntohl(iph->srcAddr.s_addr);
            _metaInfo->basicAttr.dstIP = ntohl(iph->dstAddr.s_addr);
            _metaInfo->layer3Offset = ip_offset;
            _metaInfo->layer4Offset = ip_offset + iph->getHeaderLen();

            u_int16 l4_packet_len = iph->isFragment() ? 0 : ntohs(iph->len) - iph->getHeaderLen();
            u_int8 l4_proto = iph->protocol;
            u_int8* l4h = (u_int8*)iph + iph->getHeaderLen();
            ProcessPacketL4_(l4_packet_len, l4_proto,
                l4h, _metaInfo->basicAttr.pktLen, _metaInfo);
        }
        break;

    case HO_ETH_IP6:
        if (_metaInfo->basicAttr.pktLen >= ip_offset + sizeof(ipv6_header_t))
        {
            struct ipv6_header_t *ip6 = (struct ipv6_header_t*)&packet[ip_offset];

            if (ip6->ip6_ctlun.ip6_un2.ver != 6) // This is not IPv6
                return -1;
            else
            {
                _metaInfo->basicAttr.version = 1;
                _metaInfo->basicAttr.srcIP = ip6->srcAddr.addr32[2] + ip6->srcAddr.addr32[3];
                _metaInfo->basicAttr.dstIP = ip6->dstAddr.addr32[2] + ip6->dstAddr.addr32[3];
                _metaInfo->layer3Offset = ip_offset;
                _metaInfo->layer4Offset = ip_offset + sizeof(const struct ipv6_header_t);

                u_int16 l4_packet_len = ntohs(ip6->ip6_ctlun.ip6_un1.len);
                u_int8 l4_proto = ip6->ip6_ctlun.ip6_un1.next;
                u_int8* l4h = (u_int8*)ip6 + sizeof(const struct ipv6_header_t);
                ProcessPacketL4_(l4_packet_len, l4_proto, l4h, _metaInfo->basicAttr.pktLen, _metaInfo);
            }
        }
        break;
    }

    return 0;
}

void ProcessPacketL4_(u_int16 _l4_packet_len, u_int8 _l4_proto, const u_int8 *_l4h,
                      u_int16 _rawSize, PacketMeta_t* _metaInfo)
{
    u_int16 src_port, dst_port;

    if (_l4_packet_len + _metaInfo->layer4Offset > _rawSize)
    {
        if (_rawSize > _metaInfo->layer4Offset)
        {
            _l4_packet_len = _rawSize - _metaInfo->layer4Offset;
        }
        else
        {
            _l4_packet_len = 0;
        }
    }

    if ((_l4_proto == IPPROTO_TCP) && (_l4_packet_len >= 20)) // tcp
    {
        struct tcp_header_t *tcph = (struct tcp_header_t *)_l4h;
        src_port = ntohs(tcph->srcPort);
        dst_port = ntohs(tcph->dstPort);
        _metaInfo->basicAttr.seq = tcph->getSeqNumber();
        u_int8 tcpHeaderLen = tcph->getHeaderLen();
        if (_l4_packet_len >= tcpHeaderLen)
        {
            _metaInfo->layer5payloadLen = _l4_packet_len - tcpHeaderLen;
            _metaInfo->layer5Offset = _metaInfo->layer4Offset + tcpHeaderLen;
        }
        _metaInfo->basicAttr.protocol = 0;
        _metaInfo->basicAttr.flagSYN = tcph->field0.flags.syn;
        _metaInfo->basicAttr.flagACK = tcph->field0.flags.ack;
        _metaInfo->basicAttr.flagFIN = tcph->field0.flags.fin;
        _metaInfo->basicAttr.flagRST = tcph->field0.flags.rst;
        _metaInfo->basicAttr.flagPUSH = tcph->field0.flags.psh;
        _metaInfo->basicAttr.flagURT = tcph->field0.flags.urgent;
    }
    else if ((_l4_proto == IPPROTO_UDP) && (_l4_packet_len >= sizeof(struct udp_header_t))) // udp
    {
        struct udp_header_t *udph = (struct udp_header_t *)_l4h;
        src_port = ntohs(udph->srcPort);
        dst_port = ntohs(udph->dstPort);
        _metaInfo->layer5payloadLen = _l4_packet_len - sizeof(struct udp_header_t);
        _metaInfo->layer5Offset = _metaInfo->layer4Offset + sizeof(struct udp_header_t);
        _metaInfo->basicAttr.protocol = 1;
    }
    else
    {
        src_port = dst_port = 0;
    }

    _metaInfo->basicAttr.srcPort = src_port;
    _metaInfo->basicAttr.dstPort = dst_port;

    return;
}
