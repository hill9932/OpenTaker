#include "inetaddr.h"
#include "common.h"
#include "auto_ptr.hxx"

#define UNDEFINED_PORT  13

namespace ChangQian
{
    CInetAddr::CInetAddr()
    {
        memset(&m_storage, 0, sizeof(m_storage));
    }

    CInetAddr::CInetAddr(const char *_addr, const char *_protocol)
    {
        assert(_addr);
        memset(&m_storage, 0, sizeof(m_storage));

        if (0 != makeaddr(_addr, _protocol))
            throw "Fail";
    }

    CInetAddr::CInetAddr(u_int32 _addr)
    {
        memset(&m_storage, 0, sizeof(m_storage));
        m_storage.ss_family = AF_INET;
        sockaddr_in* addr = (sockaddr_in*)&m_storage;
        addr->sin_addr.s_addr = htonl(_addr);
        addr->sin_port = UNDEFINED_PORT;
    }

    CInetAddr::CInetAddr(const in_addr& _addr)
    {
        memset(&m_storage, 0, sizeof(m_storage));

        m_storage.ss_family = AF_INET;

        sockaddr_in* addr = (sockaddr_in*)&m_storage;
        addr->sin_addr = _addr;
        addr->sin_port = UNDEFINED_PORT;
    }

    CInetAddr::CInetAddr(const sockaddr_in& _addr)
    {
        memcpy(&m_storage, &_addr, sizeof(_addr));
    }

    CInetAddr::CInetAddr(const sockaddr& _addr)
    {
        memcpy(&m_storage, &_addr, sizeof(_addr));
    }

    CInetAddr::CInetAddr(sockaddr_storage& _addr)
    {
        memcpy(&m_storage, &_addr, sizeof(_addr));
    }

    bool CInetAddr::operator == (const CInetAddr& _addr) const
    {
        return memcmp(this, &_addr, sizeof(_addr)) == 0;
    }

    bool CInetAddr::operator < (const CInetAddr& _addr) const
    {
        if (*this == _addr) return false;

        if (family() > _addr.family())
        {
            return false;
        }
        else if (family() < _addr.family())
        {
            return true;
        }

        if (family() == AF_INET)
        {
            return ((sockaddr_in*)this)->sin_addr.s_addr < ((sockaddr_in*)&_addr)->sin_addr.s_addr
                || (((sockaddr_in*)this)->sin_addr.s_addr == ((sockaddr_in*)&_addr)->sin_addr.s_addr
                && port() < _addr.port());
        }
        else
        {
            return strcmp((char*)((sockaddr_in6*)this)->sin6_addr.s6_addr, (char*)((sockaddr_in6*)&_addr)->sin6_addr.s6_addr) < 0
                || ((strcmp((char*)((sockaddr_in6*)this)->sin6_addr.s6_addr, (char*)((sockaddr_in6*)&_addr)->sin6_addr.s6_addr)) == 0
                && port() < _addr.port());
        }

        return false;
    }

    int CInetAddr::size() const
    {
        int len = 0;
        if (m_storage.ss_family == AF_INET)
            len = sizeof(struct sockaddr_in);
        else if (m_storage.ss_family == AF_INET6)
            len = sizeof(struct sockaddr_in6);

        return len;
    }

    CStdStringA CInetAddr::toString() const
    {
        stringstream    ss;

        int f = family();
        if (f == AF_INET)
        {
            sockaddr_in* listen_addr_s = (sockaddr_in*)&m_storage;
            ss << inet_ntoa(listen_addr_s->sin_addr) << ":" << ntohs(listen_addr_s->sin_port);
            return ss.str();
        }
        else if (f == AF_INET6)
        {
            sockaddr_in6* listen_addr_s = (sockaddr_in6*)&m_storage;
            char addrv6[128] = { 0 };
            inet_ntop(AF_INET6, &listen_addr_s->sin6_addr, addrv6, sizeof addrv6);
            ss << addrv6 << ":" << ntohs(listen_addr_s->sin6_port);
            return ss.str();
        }

        return "Invalid address";
    }

    CStdStringA CInetAddr::ip() const
    {
        int f = family();
        if (f == AF_INET)
        {
            sockaddr_in* listen_addr_s = (sockaddr_in*)&m_storage;
            return inet_ntoa(listen_addr_s->sin_addr);

        }
        else if (f == AF_INET6)
        {
            sockaddr_in6* listen_addr_s = (sockaddr_in6*)&m_storage;
            char addrv6[128] = { 0 };
            inet_ntop(AF_INET6, &listen_addr_s->sin6_addr, addrv6, sizeof addrv6);
            return addrv6;
        }

        return "Invalid address";
    }

    int CInetAddr::port() const
    {
        int f = family();
        if (f == AF_INET)
        {
            sockaddr_in* listen_addr_s = (sockaddr_in*)&m_storage;
            return ntohs(listen_addr_s->sin_port);
        }
        else if (f == AF_INET6)
        {
            sockaddr_in6* listen_addr_s = (sockaddr_in6*)&m_storage;
            return ntohs(listen_addr_s->sin6_port);
        }

        return -1;
    }

    CStdStringA CInetAddr::hostname() const
    {
        char name[128] = "invalid";
        int f = family();
        struct hostent *hp = NULL;

        if (f == AF_INET)
        {
            sockaddr_in* client_addr_s = (sockaddr_in*)&m_storage;
            hp = gethostbyaddr((char *)&client_addr_s->sin_addr,
                sizeof client_addr_s->sin_addr,
                client_addr_s->sin_family);

            snprintf_t(name, sizeof name, "%s (%s:%u)",
                hp ? hp->h_name : "Anonymous",
                inet_ntoa(client_addr_s->sin_addr),
                (unsigned)ntohs(client_addr_s->sin_port));
        }
        else if (f == AF_INET6)
        {
            sockaddr_in6* client_addr_s = (sockaddr_in6*)&m_storage;
            hp = gethostbyaddr((char *)&client_addr_s->sin6_addr,
                sizeof client_addr_s->sin6_addr,
                client_addr_s->sin6_family);

            char addrv6[128] = { 0 };
            inet_ntop(AF_INET6, &client_addr_s->sin6_addr, addrv6, sizeof addrv6);
            snprintf_t(name, sizeof name, "%s (%s:%u)",
                hp ? hp->h_name : "Anonymous",
                addrv6,
                (unsigned)ntohs(client_addr_s->sin6_port));
        }

        return name;
    }

    bool CInetAddr::isGroupAddress()
    {
        int addr = htonl(((sockaddr_in*)this)->sin_addr.s_addr);
        return addr > 0xE00000FF && addr <= 0xEFFFFFFF;
    }

    int CInetAddr::getLocalAddrs(local_addr_t** _addrs, int _family, int _type)
    {
        int ret;
        char buffer[256] = { 0 };
        ret = gethostname(buffer, sizeof(buffer));
        ON_ERROR_LOG_LAST_ERROR_AND_DO(ret, != , 0, return err);


        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = _family;
        hints.ai_socktype = _type;

        ret = getaddrinfo(buffer, NULL, &hints, &res);
        ON_ERROR_LOG_LAST_ERROR_AND_DO(ret, == , -1, return err);

        if (*_addrs) *_addrs = NULL;
        struct addrinfo *tmp = res;
        while (tmp)
        {
            local_addr_t *addr = new local_addr_t;
            addr->addr_len = tmp->ai_addrlen;
            memcpy(&addr->addr, tmp->ai_addr, addr->addr_len);
            addr->next = NULL;

            if (!(*_addrs))
            {
                *_addrs = addr;
            }
            else
            {
                addr->next = *_addrs;
                *_addrs = addr;
            }

            tmp = tmp->ai_next;
        }

        freeaddrinfo(res);

        return 0;
    }

    void CInetAddr::freeLocalAddrs(local_addr_t*& _addr)
    {
        local_addr_t *tmp;

        while (_addr)
        {
            tmp = _addr;
            _addr = _addr->next;
            delete tmp;
        }
    }

    int CInetAddr::makeaddr(const char* _addr, const char* _protocol)
    {
        if (!_addr)    return -3;

        AutoFree<char> inp_addr(strdup(_addr));
        const char *host_part = strtok(inp_addr, ":");
        const char *port_part = strtok(NULL, "\n");
        struct sockaddr_in *ap = (sockaddr_in *)this;
        struct hostent *hp = NULL;
        struct servent *sp = NULL;
        char *cp = NULL;
        long lv;

        if (!host_part) host_part = "*";
        if (!port_part) port_part = "*";
        if (!_protocol)  _protocol = "tcp";

        ap->sin_family = AF_INET;
        ap->sin_port = 0;
        ap->sin_addr.s_addr = INADDR_ANY;

        //
        // Fill in the host address:
        //
        if (strcmp(host_part, "*") == 0);   // Leave as INADDR_ANY
        else if (isdigit(*host_part) &&
            host_part[0] != '0')       // Numeric IP address
        {
            if (inet_pton(AF_INET, host_part, &ap->sin_addr) <= 0)
                return -1;
            /*      ap->sin_addr.s_addr = inet_addr(host_part);
                  if (ap->sin_addr.s_addr == INADDR_NONE) return -1;
                  */
        }
        else    // Assume a hostname
        {
            hp = gethostbyname(host_part);
            if (!hp)                        return -1;
            if (hp->h_addrtype != AF_INET ||
                hp->h_addrtype != AF_INET6)  return -1;

            ap->sin_family = hp->h_addrtype;

            if (hp->h_addrtype == AF_INET)
                ap->sin_addr = *(struct in_addr *) hp->h_addr_list[0];
            else if (hp->h_addrtype == AF_INET6)
                ((struct sockaddr_in6*)ap)->sin6_addr = *(struct in6_addr *) hp->h_addr_list[0];
        }

        //
        // Process an optional port
        //
        if (!strcmp(port_part, "*"));       // Leave as wild (zero)
        else if (isdigit(*port_part))       // Process numeric port
        {
            lv = strtol(port_part, &cp, 10);
            if (cp != NULL && *cp)          return -2;  // there is invalid character
            if (lv < 0L || lv >= 65535)     return -2;  // the port number is invalid

            ap->sin_port = htons((short)lv);
        }
        else    // Lookup the service
        {
            sp = getservbyname(port_part, _protocol);
            if (!sp)                        return -2;
            ap->sin_port = (short)sp->s_port;
        }

        return 0;
    }

}
