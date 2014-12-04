#include "inetaddr.h"
#include "common.h"
#include "string_.h"

#define UNDEFINED_PORT  0

namespace ChangQian
{
    CInetAddr::CInetAddr()
    {
        memset(&m_storage, 0, sizeof(m_storage));
    }

    CInetAddr::CInetAddr(const char *_addr)
    {
        assert(_addr);
        memset(&m_storage, 0, sizeof(m_storage));

        if (0 != makeAddr(_addr))
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

    int CInetAddr::makeAddr(const char* _addr)
    {
        if (!_addr)    return -1;

        struct in_addr addr4  = { 0 };
        struct in6_addr addr6 = { 0 };
        bool isV6 = false;

        vector<CStdString> v;
        LiangZhu::StrSplit(_addr, ":", v);
        if (strstr(_addr, "::") || v.size() > 2)
            isV6 = true;

        if (isV6) // IPv6
        {
            if (inet_pton(AF_INET6, _addr, &addr6) == 1)
            {
                sockaddr_in6* inAddr6 = (sockaddr_in6*)&m_storage;
                inAddr6->sin6_family = AF_INET6;
                memcpy(&inAddr6->sin6_addr, &addr6, sizeof(addr6));
            }
        }
        else
        {
            if (inet_pton(AF_INET, v[0], &addr4) == 1)
            {
                sockaddr_in* inAddr4 = (sockaddr_in*)&m_storage;
                inAddr4->sin_family = AF_INET;
                inAddr4->sin_addr.s_addr = addr4.s_addr;
                if (v.size() == 2)
                    inAddr4->sin_port = htons(atoi(v[1]));
            }
        }

        return 0;
    }

}
