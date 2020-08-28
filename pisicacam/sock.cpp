/**
# Copyright (C) 2007-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <time.h>
#include "sock.h"
#include "main.h"
#include "config.h"

#ifndef _WIN32
#   include <ifaddrs.h>
#endif //
//#include "main.h"
//#include "logs.h"
//-----------------------------------------------------------------------------
unsigned long       sock::_tout = 8000;



//-----------------------------------------------------------------------------
bio_unblock::bio_unblock(sock* sock, int bl):_sk(sock),_bl(_sk->_blocking){
    sock->set_blocking(bl);
}
bio_unblock::~bio_unblock(){
    _sk->set_blocking(_bl);
}

//-----------------------------------------------------------------------------
void sock::Init()
{
#ifdef WIN32
    WSADATA     WSAData;
    WSAStartup (MAKEWORD(1,1), &WSAData);
#endif
}

SADDR_46 sock::sip2ip(const char* sip, uint16_t port)
{
    int a,b,c,d,e=port;

    if(!isdigit(sip[0]))
        return SADDR_46((u_int32_t)0, 0);
    if(strchr(sip,':') && 5==::sscanf(sip,"%u.%u.%u.%u:%d",&a,&b,&c,&d,&e))
    {
        a&=0xff;
        b&=0xff;
        c&=0xff;
        d&=0xff;
        return SADDR_46(u_int32_t(a<<24|b<<16|c<<8|d), (uint16_t)e);
    }
    if(4==::sscanf(sip,"%u.%u.%u.%u",&a,&b,&c,&d))
    {
        a&=0xff;
        b&=0xff;
        c&=0xff;
        d&=0xff;
        return SADDR_46(u_int32_t(a<<24|b<<16|c<<8|d), 0);
    }
    return SADDR_46((u_int32_t)0, 0);
}
//-----------------------------------------------------------------------------

void sock::Uninit()
{
#ifdef WIN32
    WSACleanup();
#endif
}


//-----------------------------------------------------------------------------
bool sock::CTime(void* , unsigned long time)
{
    return time < sock::_tout;
}

SADDR_46 sock::dnsgetip(const char* sip, char* out, int port)
{
    if(isdigit(sip[0]))
    {
        int a=256,b=256,c=256,d=256,e=port;

        if(strchr(sip,':'))
            ::sscanf(sip,"%d.%d.%d.%d:%d",&a,&b,&c,&d,&e);
        else
            ::sscanf(sip,"%d.%d.%d.%d",&a,&b,&c,&d);

        if(a<256 && b<256 && c<256 && d<256)
        {
            SADDR_46 p = sock::sip2ip(sip);
            if(p.ip4() != 0)
            {
                return p;
            }
        }
    }

    char loco[128];

    ::strcpy(loco,sip);
    char* pcol=::strrchr(loco,':');
    if(pcol){
        port=::atoi(pcol+1);
        *pcol=0;
    }

    struct hostent*  hostent = ::gethostbyname(loco);
    if(hostent==0)
    {
        long dwbo = inet_addr(sip);
        hostent = gethostbyaddr((char*)&dwbo, (int)sizeof(dwbo), AF_INET );
    }
    if(hostent)
    {
        SADDR_46	sin;
        ::memcpy((char*)&(sin.sin_addr), hostent->h_addr, hostent->h_length);
        sin.commit();
        if(out)
        {
            ::sprintf(out,"%s:%d", inet_ntoa((struct in_addr)sin.sin_addr),port);
        }
        sin.set_port(port);
        return sin;
    }
    return SADDR_46((u_int32_t)0,0);
}

bool sock::dnsgetname(u_int32_t uip, char* out)
{

    struct sockaddr_in addr;
    struct hostent *h;

    addr.sin_family =   AF_INET;
    addr.sin_port   =   htons(80);
    addr.sin_addr.s_addr    =   uip;

    h=gethostbyaddr((char*)&(addr.sin_addr),
                    sizeof(struct in_addr),
                    AF_INET );

    if(h!=NULL)
    {
        strcpy(out, h->h_name);
        return true;
    }
    return false;

}

bool sock::dnsgetnameinfo(const SADDR_46& uip, char* out)
{
    char    node[128] = {0};

    int res = getnameinfo((struct sockaddr*)&uip, uip.rsz(), node, sizeof(node), NULL, 0, 0);
    if(res==0)
    {

        strcpy(out, node);
        return true;
    }
    return false;
}



//-----------------------------------------------------------------------------
sock::sock()
{
    _thesock     = -1;
    _io         = 0;
    _error       = 0;
    _blocking    = 0;
    _buffers[0] = 0;
    _buffers[1] = 0;
};

//-----------------------------------------------------------------------------
sock::~sock()
{
    destroy();
};

void  sock::attach(int s)
{
    _thesock = s;
}

//-----------------------------------------------------------------------------
int sock::set_blocking(const unsigned long block)
{
    _blocking = block;
    if(isopen())
    {
        int flags = fcntl(_thesock, F_GETFL, 0);
        if(!block)
            fcntl(_thesock, F_SETFL, flags | O_NONBLOCK);
        else
            fcntl(_thesock, F_SETFL, flags & ~O_NONBLOCK);
    }
    return 0;
}

//-----------------------------------------------------------------------------
SOCKET sock::create(int , int , const char* )
{
    return (SOCKET)-1;
}

//-----------------------------------------------------------------------------
SOCKET sock::create(const SADDR_46&, int)
{
    return (SOCKET)-1;
}


//-----------------------------------------------------------------------------
char* sock::GetLocalIP(const char* allow)
{
    static char localip[2048];

    memset(localip,0,sizeof(localip));
#ifdef _WIN32
    sockaddr_in locSin = {0};
    hostent*    pHe;
    char        szBuf[256];
    unsigned long       dwSize = __sfz(char, szBuf);

    locSin.sin_family = AF_INET;
    if(-1 == gethostname((char*)szBuf, (int)dwSize))
    {
        sprintf(localip, "ghname err %d", errno);
        return localip;
    }
    pHe = gethostbyname(szBuf);
    if (pHe == NULL)
    {
        sprintf(localip, "ghbyname(%s) err %d", szBuf, errno);
        return localip;
    }
    ::memcpy((char*)&(locSin.sin_addr), pHe->h_addr,pHe->h_length);
    ::sprintf(localip,"%s", inet_ntoa((struct in_addr)locSin.sin_addr));

#else
    char    tmp[128];
    struct  ifaddrs* ifAddrStruct = 0;
    struct  ifaddrs* ifAddrStructo = 0;
    void    *tmpAddrPtr = 0;

    getifaddrs(&ifAddrStruct);
    ifAddrStructo=ifAddrStruct;
    while (ifAddrStruct != 0)
    {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET && strcmp(ifAddrStruct->ifa_name, "lo0")!=0)
        {
            tmpAddrPtr = (void*)&((SADDR_46 *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, tmp, (sizeof(tmp)-1));
            if(strlen(localip) + strlen(tmp) < (sizeof(localip)-1) )
            {
                if(::strstr(allow, tmp))
                {
                    ::strcpy(localip, tmp);
                    break;
                }
            }
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    freeifaddrs(ifAddrStructo);
#endif //
    return localip;
}

//-----------------------------------------------------------------------------
int sock::get_option(int option)
{
    if(isopen())
    {
        int optionVal;
        socklen_t optLen = sizeof(optionVal);
        if(0 == getsockopt(_thesock, SOL_SOCKET, option, (char*)&optionVal, &optLen))
            return optionVal;
    }
    return -1;
}


//-----------------------------------------------------------------------------
//err = ::setsockopt(_thesock, SOL_SOCKET, SO_SNDBUF, (char *) &bfSz, sizeof(bfSz));
int sock::set_option(int option, int optionval)
{
    if(isopen())
        return  setsockopt (_thesock, SOL_SOCKET, option, &optionval, sizeof (optionval));
    return -1;
}

//-----------------------------------------------------------------------------
int  sock::select_receive(char* buff, int length, int toutms, int wait)
{
    return select_receive((unsigned char*) buff, length, toutms,  wait);
}

//-----------------------------------------------------------------------------
int  sock::select_receive(unsigned char* buff, int length, int toutms , int wait)
{
    int      bytes = -1;
    timeval  tv   = {0, toutms*1000};
    fd_set   rdSet;
    time_t   fut = time(0) + wait;
    _error = 0;

    do{
        FD_ZERO(&rdSet);
        FD_SET(_thesock, &rdSet);
        int nfds = (int)_thesock+1;
        tv.tv_sec=0;
        tv.tv_usec=toutms*1000;
        int sel = ::select(nfds, &rdSet, 0, 0, &tv);
        if(sel > 0 && FD_ISSET(_thesock, &rdSet))
        {
            FD_CLR(_thesock, &rdSet);
            bytes = receive(buff, length);
            if(0 == bytes)
            {
                return 0; //remote closed the socket
            }
            return bytes;
        }
        else if(sel < 0)
        {
            _error = errno;
            return 0; //select error. we close
        }

    }while(time(0)<fut && bytes<0 && wait);

    return -1; // no data this time
}


tcp_sock::~tcp_sock()
{
}

//-----------------------------------------------------------------------------
char* tcp_sock::ssock_addrip()
{
    ::strcpy(_sip, inet_ntoa((struct in_addr)_remote_sin.sin_addr));
    return _sip;
}


//-----------------------------------------------------------------------------
int tcp_sock::getsocketport()const
{
    return (int)htons(_remote_sin.sin_port);
}

//-----------------------------------------------------------------------------
const SADDR_46& tcp_sock::getsocketaddr()const
{
    return _remote_sin;
}

//-----------------------------------------------------------------------------
char* tcp_sock::getsocketaddr_str(char* pAddr)const
{
    ::strcpy(pAddr, inet_ntoa((struct in_addr)_remote_sin.sin_addr));
    return pAddr;
}

int tcp_sock::receiveall(unsigned char* buff, const int length)
{
    int shot = 0;
    int toreceive = length;
    int got   = 0;

    _error = 0;
    while(toreceive)
    {
        shot = receive((unsigned char*)(buff+got), toreceive);
        if(shot <= 0)
            break;
        toreceive -= shot;
        got += shot;
    }
    return got;

}

//-----------------------------------------------------------------------------
//returns 0 for success
int     tcp_sock::sendall(const unsigned char* buff, int length, int tout)
{
    int shot = 0;
    int sent = 0;

    _error = 0;
    int toutnonblock = (tout/10) + 1; //8 sec
    while(length > 0)
    {
        shot = ::send(_thesock,(char *)buff+sent, length, MSG_NOSIGNAL);
        if(shot <= 0)
        {
            _error = errno;
            if(_error==0)break;
            if(_error == EAGAIN || _error==11)
            {
                if(--toutnonblock < 0)
                {
                    break; //
                }
                usleep(10000); // 10 milliseconds
                continue;
            }
            this->destroy();
            break;
        }
        length -= shot;
        sent   += shot;
    }
    return sent;
}

//-----------------------------------------------------------------------------
int tcp_sock::send(const char* buff, const int length, int , const char* )
{
    _io &= ~0x2;
    return send((unsigned char*)buff, length, 0 , 0 );
}
//-----------------------------------------------------------------------------
int tcp_sock::send(const unsigned char* buff, const int length, int , const char* )
{
    _error = 0;
    assert(length);
    _io &= ~0x2;
    return ::send(_thesock,(char *)buff, length, 0);
}

//-----------------------------------------------------------------------------
int tcp_sock::receive(char* buff, int length, int , const char* )
{
    _io &= ~0x1;
    _error = 0;
    int rb = ::recv(_thesock,(char *)buff, length, 0);
    if(rb<0)
    {
        _error = errno;
        if(errno==EAGAIN || errno== EWOULDBLOCK)
        {
            return -1;
        }
        return 0;
    }
    return rb;
}

//-----------------------------------------------------------------------------
int tcp_sock::receive(unsigned char* buff, int length, int , const char* )
{
    _error = 0;
    _io &= ~0x1;
    int rb = ::recv(_thesock,(char *)buff, length, 0);
    if(rb<0)
    {
        _error = errno;
        if(errno==EAGAIN || errno== EWOULDBLOCK)
        {
            return -1;
        }
        return 0;
    }
    return rb;
}

//-----------------------------------------------------------------------------
bool sock::destroy(bool)
{
    bool b=false;
    _io = 0;
    if((int)_thesock > 0)
    {
        int k = 32;
        while(-1==::close(_thesock) && --k>0)
        {
            _error = errno;
            usleep(0xFF);
        }
        b=true;
    }
    _thesock = -1;
    return b;
}

bool    tcp_srv_sock::destroy(bool emptyit)
{
    if(!isopen())  return false;
    int         n = 0;

    shutdown(_thesock, 0x3);
    if(emptyit)
    {
        char        buff[64];
        bio_unblock bl(this);
        do{
            n = recv(_thesock,(char *)buff, 64, 0);
        } while (n > 0);
        shutdown(_thesock, 0x3);
    }
    return sock::destroy(emptyit);
}

tcp_cli_sock::tcp_cli_sock()
{
    _hostent    = 0;
    _connecting = 0;
    _connected  = 0;
    _connecting = 0;
}

tcp_cli_sock::tcp_cli_sock(const tcp_cli_sock& s):tcp_sock()
{
    _thesock    = s._thesock;
    _error      = s._error;
    _blocking   = s._blocking;
    _connected  = s._connected;
    _connecting  = s._connecting;
    _local_sin   = s._local_sin;
    _remote_sin  = s._remote_sin;

}

tcp_cli_sock& tcp_cli_sock::operator=(const tcp_cli_sock& s)
{
    if(this != &s)
    {
        _thesock    = s._thesock;
        _error      = s._error;
        _blocking   = s._blocking;
        _connected  = s._connected;
        _connecting  = s._connecting;
        _local_sin = s._local_sin;
        _remote_sin = s._remote_sin;
    }
    return *this;
}

bool tcp_cli_sock::is_really_connected()
{
    if(_thesock>0)
    {
        struct      sockaddr saddr;
        socklen_t   sz = 0;           // ; = sizeof(_remote_sin);
        if(0 == getpeername(_thesock, (struct sockaddr*)&saddr , &sz))
        {
            //::memcpy(&_remote_sin, &saddr, sizeof(_remote_sin));
            _connected=1;
            return true;
        }
    }
    _connected=0;
    return false;
}


tcp_cli_sock::~tcp_cli_sock()
{
    destroy();
}


bool    tcp_cli_sock::destroy(bool emptyit)
{
    if(!isopen())   return false;
    int         n = 0;
    _connecting = 0;
    _connected = 0;
    shutdown(_thesock, 0x3);
    if(emptyit)
    {
        char        buff[64];
        bio_unblock bl(this);
        do{
            n = recv(_thesock,(char *)buff,64, 0);
        }while (n > 0);
        shutdown(_thesock, 0x3);
    }

    return sock::destroy(emptyit);
}

/*
//-----------------------------------------------------------------------------
const char*  tcp_cli_sock::tcp_cli_sock()
{
    ::t_sprintf(_sip,"%d.%d.%d.%d",
          (int)(_remote_sin.sin_addr.s_addr&0xFF),
          (int)(_remote_sin.sin_addr.s_addr&0xFF00)>>8,
          (int)(_remote_sin.sin_addr.s_addr&0xFF0000)>>16,
          (int)(_remote_sin.sin_addr.s_addr&0xFF000000)>>24);
    return _sip;
}
*/
//-----------------------------------------------------------------------------
tcp_sock::tcp_sock()
{
    _sip[0] = 0;
}

//-----------------------------------------------------------------------------
SOCKET tcp_sock::create(int , int opt, const char* )
{
    _error = 0;
    ::memset(&_local_sin,0,sizeof(_local_sin));
    ::memset(&_remote_sin,0,sizeof(_remote_sin));
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
        _error = errno;
    if(opt)
    {
        int one = 1;
        ::setsockopt(_thesock, SOL_SOCKET, /*SO_REUSEADDR*/opt, (char *)&one, sizeof(one));
        if(_buffers[0] && _buffers[1])
        {
            set_option(SO_SNDBUF,_buffers[0]);
            set_option(SO_RCVBUF,_buffers[1]);
            _buffers[0] = _buffers[1] = 0;
        }
    }
    return _thesock;
}

//-----------------------------------------------------------------------------
SOCKET tcp_sock::create(const SADDR_46& r, int opt)
{
    _error = 0;
    _local_sin.clear();
    _remote_sin = r;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
        _error = errno;
    if(opt)
    {
        int one = 1;
        ::setsockopt(_thesock, SOL_SOCKET, /*SO_REUSEADDR*/opt, (char *)&one, sizeof(one));
        if(_buffers[0] && _buffers[1])
        {
            set_option(SO_SNDBUF,_buffers[0]);
            set_option(SO_RCVBUF,_buffers[1]);
            _buffers[0] = _buffers[1] = 0;
        }
    }
    return _thesock;
}


//-----------------------------------------------------------------------------
int tcp_sock::listen(int maxpending)
{
    _error = 0;
    int rv = ::listen(_thesock, maxpending);
    if((int)-1 == rv)
    {
        _error = errno;
        return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
tcp_srv_sock::tcp_srv_sock() {}

tcp_srv_sock::~tcp_srv_sock() {}
//-----------------------------------------------------------------------------
SOCKET tcp_srv_sock::create(int port, int opt, const char* iface)
{
    _error = 0;
    ::memset(&_local_sin,0,sizeof(_local_sin));
    ::memset(&_remote_sin,0,sizeof(_remote_sin));
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_thesock <= 0)
    {
        _error = errno;
        return (SOCKET)-1;
    }
    _local_sin.sin_family		= AF_INET;
    if(iface)
        inet_aton(iface, &_local_sin.sin_addr);
    else
        _local_sin.sin_addr.s_addr	= htonl(INADDR_ANY);
    _local_sin.sin_port		    = htons(n_port = port);
    // option before bind will allow recreation on same SIN when closed imediatelly
    if(opt)
    {
        int one = 1;
        ::setsockopt(_thesock, SOL_SOCKET, /*SO_REUSEADDR*/opt, (char *)&one, sizeof(one));
    }
    if(::bind(_thesock, (struct sockaddr*)&_local_sin, _local_sin.rsz()) < 0)
    {
        _error =  errno;
        destroy();
        return (SOCKET)-1;
    }
    return _thesock;
}

//-----------------------------------------------------------------------------
SOCKET tcp_srv_sock::create(const SADDR_46& r, int opt)
{
    _error = 0;
    _local_sin.clear();
    _remote_sin = r;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_thesock <= 0)
    {
        _error = errno;
        return (SOCKET)-1;
    }
    _local_sin.sin_family		= AF_INET;
    _local_sin.sin_addr.s_addr	= htonl(INADDR_ANY);
    if(::bind(_thesock,(struct sockaddr*)&_local_sin,sizeof(_local_sin)) < 0)
    {
        _error =  errno;
        destroy();
        return (SOCKET)-1;
    }
    if(opt)
    {
        int one = 1;
        ::setsockopt(_thesock, SOL_SOCKET, /*SO_REUSEADDR*/opt, (char *)&one, sizeof(one));
    }
    return _thesock;
}


//-----------------------------------------------------------------------------
SOCKET tcp_srv_sock::accept(tcp_cli_sock& cliSock)
{
    _error = 0;
    ::memset(&cliSock._remote_sin,0,sizeof(cliSock._remote_sin));
    socklen_t clilen = (socklen_t)sizeof(cliSock._remote_sin);
    cliSock._thesock = ::accept(_thesock,
                                (struct sockaddr*)&cliSock._remote_sin,
                                &clilen);
    if((int)cliSock._thesock <=0 )
    {
        //prinf("accept " << errno <<", " << strerror(errno));
        _error = errno;
        cliSock.destroy();
        return 0;
    }
    if(_buffers[0] && _buffers[1])
    {
        set_option(SO_SNDBUF,_buffers[0]);
        set_option(SO_RCVBUF,_buffers[1]);
    }
    return cliSock._thesock;
}

SOCKET tcp_cli_sock::create(const SADDR_46&, int opt)
{
    return create(0,opt,0);
}

//-----------------------------------------------------------------------------
SOCKET tcp_cli_sock::create(int , int opt, const char*)
{
    _hostent = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_thesock <= 0)
    {
        _error = errno;
        return -1;
    }
    if(opt)
    {
        int one = 1;
        ::setsockopt(_thesock, SOL_SOCKET, /*SO_REUSEADDR*/opt, (char *)&one, sizeof(one));
    }
#if defined SO_NOSIGPIPE
    ::setsockopt(_thesock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    return _thesock;
}

//as send -1 continue, 0 closed -cannot 1 ok

int tcp_cli_sock::try_connect(const char* sip, int port, int tout)
{
    assert((int)_thesock > 0); // create first

    _hostent = ::gethostbyname(sip);
    if(_hostent==0)
    {
        long dwbo = inet_addr(sip);
        _hostent = gethostbyaddr((char*)&dwbo, (int)sizeof(dwbo), AF_INET );
    }
    if(!_hostent)
    {
        return 0;
    }
    _connecting = 0;
    _error      = 0;
    ::memcpy((char*)&(_remote_sin.sin_addr), _hostent->h_addr, _hostent->h_length);
    this->set_blocking(_blocking);
    _remote_sin.sin_family		= AF_INET;
    _remote_sin.sin_port		= htons(port);

    if(tout)
    {
        destroy();
        return raw_connect(_remote_sin,tout);
    }

    if(-1 == ::connect(_thesock,
                       (const struct sockaddr*)&_remote_sin,
                       _remote_sin.rsz()))
    {
        _error = errno;
        if(_error==EINPROGRESS || _error == WOULDBLOCK)
        {
            _connecting = 1; //in progress
            return -1; //in progress
        }
        return 0;
    }
    _connecting = 0;
    return _thesock;
}

/*
int tcp_cli_sock::raw_connect(const char* suip4, int port)
{
    if(_ttatoi(suip4))
    {
        return raw_connect(htonl(inet_addr(suip4)), port);
    }
    sockaddr_in locSin;
    _hostent = ::gethostbyname(suip4);

    ::memcpy((char*)&(locSin.sin_addr), _hostent->h_addr, _hostent->h_length);
    return raw_connect(htonl(inet_addr(Ip2str(locSin))), port);
}
*/
int tcp_cli_sock::raw_connect(u_int32_t ip4,  int port)
{
    SADDR_46 sadr(ip4,port);
    return raw_connect(sadr);
}


// 0 no connection, >0 OK, -1 in progress
int tcp_cli_sock::raw_connect(const SADDR_46&  saddr, int tout)
{
    _error   = 0;
    _connecting = 0;
    _error   = 0;

    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
    {
        _error = errno;
        return 0; //closed
    }
    if(_buffers[0] && _buffers[1])
    {
        set_option(SO_SNDBUF,_buffers[0]);
        set_option(SO_RCVBUF,_buffers[1]);
    }
    bio_unblock  ul(this);
    _remote_sin = saddr;
    int rv = ::connect(_thesock, (const struct sockaddr*)&_remote_sin, _remote_sin.rsz());
    if(-1 == rv)
    {
        _error = errno;
        if(_error==EINPROGRESS || _error == WOULDBLOCK)
        {
            if(tout)
            {
                fd_set  fdWr;
                time_t  t = time(0);
                while(time(0) - t < tout)
                {

                    FD_ZERO(&fdWr);
                    FD_SET((unsigned int)_thesock, &fdWr);
                    int     nfds = (int)_thesock+1;
                    timeval tv = {0,0xFFF};

                    nfds = ::select(nfds,0,&fdWr,0,&tv);
                    if(nfds > 0)
                    {
                        if(FD_ISSET(_thesock, &fdWr))
                        {
                            return _thesock;   // no error
                        }
                    }
                    usleep(0xFFF);
                    FD_ZERO(&fdWr);
                }
            }
            _connecting=1;
            return -1;
        }
        return 0;//error
    }
    return _thesock; //connected
}

void      tcp_cli_sock::raw_sethost(const SADDR_46& uip4)
{
    _remote_sin = uip4;
}

int     tcp_cli_sock::raw_connect_sin()
{
    _error = 0;
    _connecting = 0;
    _error = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
    {
        _error = errno;
        return 0;
    }
    if(_buffers[0] && _buffers[1])
    {
        set_option(SO_SNDBUF,_buffers[0]);
        set_option(SO_RCVBUF,_buffers[1]);
        _buffers[0] = _buffers[1] = 0;
    }
    _connecting = 0;
    bio_unblock bl(this);
    int rv = ::connect(_thesock, (const struct sockaddr*)&_remote_sin, _remote_sin.rsz());
    if(-1 == rv)
    {
        _error = errno;
        if(_error==EINPROGRESS || _error == WOULDBLOCK)
        {
            _connecting = 1;
            return -1;
        }
        return 0;
    }
    return _thesock;
}

//-----------------------------------------------------------------------------
int tcp_cli_sock::i4connect(const SADDR_46&  addr, CancelCB cbCall, void* pUser)
{
    int             err;
    fd_set          fdWr;

    if((int)_thesock != (int)-1)
    {
        destroy();
    }
    _error = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
    {
        _error = errno;
        return 0;
    }
    _remote_sin = addr;

    if(_hostent==0)
    {
        _hostent = gethostbyaddr((char*)&addr, (int)sizeof(long), AF_INET );
    }

    // non blocking node couse we can cancel it by Cancel f_call
    if(0==cbCall) cbCall = sock::DefCBCall;

    bio_unblock bl(this);
    time_t  ti = time(0);
    _connecting = 0;
    err = ::connect(_thesock, (const struct sockaddr*)&_remote_sin, _remote_sin.rsz());
    if(err==-1 )
    {
        _error = errno;
    }
    if(_error==EINPROGRESS || _error == WOULDBLOCK)
    {
        _connecting = 1;
        while(cbCall(pUser, time(0)-ti))
        {
            int     nfds = (int)_thesock+1;
            timeval tv = {1, 0};

            FD_ZERO(&fdWr);
            FD_SET((unsigned int)_thesock, &fdWr);
            err = ::select(nfds,0,&fdWr,0,&tv);
            if(err > 0)
            {
                if(FD_ISSET(_thesock, &fdWr))
                {
                    _connecting = 0;
                    return _thesock;   // no error
                }
            }
            usleep(10000);
            FD_ZERO(&fdWr);
        }
    }
    if(err==-1)
    {
        destroy();
        return 0;
    }
    _connecting = 0;
    return _thesock;
}

//-----------------------------------------------------------------------------
int tcp_cli_sock::s4connect(const char* sip, int port, CancelCB cbCall, void* pUser)
{
    int             err;
    fd_set          fdWr;
    timeval         tv = {1, 0};

    if(0==cbCall)
        cbCall = sock::DefCBCall;

    if((int)_thesock != (int)-1)
    {
        destroy();
    }
    _error = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_STREAM, 0);
    if((int)_thesock < 0)
    {
        _error = errno;
        return 0;
    }
    if(0==_hostent)
        _hostent = ::gethostbyname(sip);
    _remote_sin.sin_family		= AF_INET;
    _remote_sin.sin_port		= htons(port);
#ifdef _WIN32
    _remote_sin.sin_addr.s_addr	= inet_addr(sip);
#else
    inet_aton(sip, &_remote_sin.sin_addr);
#endif //
    bio_unblock bl(this);

    _connecting = 0;
    err = ::connect(_thesock, (const struct sockaddr*)&_remote_sin, _remote_sin.rsz());
    if(err==-1)
    {
        COUT_( "connect() "<< strerror(errno) );
        _error = errno;
    }
    if(_error == EINPROGRESS || _error == WOULDBLOCK)
    {
        _connecting = 1;
        if(pUser==(void*)-1)
        {
            return -1;
        }

        COUT_( "connect() in progress");

        int nfds = (int)_thesock+1;
        time_t     ti = time(0) + 8;
        while(time(0)<ti)
        {
            FD_ZERO(&fdWr);
            FD_SET((unsigned int)_thesock, &fdWr);
            tv.tv_sec = 1;
            err = ::select(nfds,0,&fdWr,0,&tv);
            if(err > 0)
            {
                if(FD_ISSET(_thesock, &fdWr))
                {
                    COUT_( "really connected");
                    _connecting = 0;
                    return _thesock;   // no error
                }
            }
            usleep(10000);
        }
    }
    if(err==-1)
    {
        COUT_( "connect() desotryed");
        _connecting = 0;
        destroy();
        return 0;
    }
    COUT_( "connect() in progress return");
    _connecting = 0;
    return _thesock;
}

//-----------------------------------------------------------------------------
int tcp_cli_sock::connect(const char* sip, int port, CancelCB cbCall, void* pUser)
{
    _connecting = 1;
    _error = 0;
    sockaddr_in    locSin;
    _hostent = ::gethostbyname(sip);
    if(_hostent==0)
        _hostent = gethostbyaddr(sip, (int)strlen(sip), AF_INET );
    if(_hostent)
    {
        ::memcpy((char*)&(locSin.sin_addr), _hostent->h_addr, _hostent->h_length);
        return s4connect(Ip2str(locSin.sin_addr.s_addr),
                         port, cbCall, pUser);
    }
    _connecting = 0;
    _error = errno;
    return 0;
}


//-----------------------------------------------------------------------------
bool sock::DefCBCall(void*, unsigned long dw)
{
    return dw < sock::_tout;
}


//-----------------------------------------------------------------------------
SOCKET udp_sock::create(const SADDR_46& r, int proto)
{
    _error    = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_DGRAM, proto);
    if((int)-1 == (int)_thesock)
        return -1;
    _local_sin = r;
    return _thesock;
}


//-----------------------------------------------------------------------------
SOCKET udp_sock::create(int port, int proto, const char* addr)
{
    _error    = 0;
    assert(_thesock<=0);
    _thesock = ::socket(AF_INET, SOCK_DGRAM, proto);
    if((int)-1 == (int)_thesock)
        return -1;
    ::memset(&_local_sin, 0, sizeof(_local_sin));

    _local_sin.sin_family        = AF_INET;
    _local_sin.sin_addr.s_addr   = addr ? inet_addr(addr): htonl(INADDR_ANY);
    _local_sin.sin_port          = htons(port);
    return _thesock;
}

int  udp_sock::bind(const char* addr, int port)
{
    memset(&_local_sin,0,sizeof(_local_sin));
    if(addr)
        _local_sin.sin_addr.s_addr = inet_addr(addr);
    else
        _local_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if(port)
        _local_sin.sin_port = htons(port);

    assert(_local_sin.sin_port > 0); //did you pass in at create the addr and port
    if(::bind(_thesock,(struct sockaddr *)&_local_sin, _local_sin.rsz()))
    {
        printf("udp-sock-bind-error\n");
        perror("bind error \n");
        _error = errno;
        return -1;
    }
    return _thesock;
}

//-----------------------------------------------------------------------------
int  udp_sock::send(const char* buff, const int length, int port, const char* ip)
{
    return send((const unsigned char*)buff, length, port, ip);
}

//-----------------------------------------------------------------------------
int  udp_sock::send(const unsigned char* buff, const int length, int port, const char* ip)
{
    int snd = -1;
    _error  = 0;

    if(_connected)
    {
        snd = ::send(_thesock,(char *)buff, length, 0);
    }
    else if(_bind)
    {
        ////printf("<-to %s : %d\n", IP2STR(_remote_sin.sin_addr.s_addr), htons(_remote_sin.sin_port));

        snd = ::sendto(_thesock, (char*)buff, length, 0,
                       (struct sockaddr  *) &_remote_sin,
                       _remote_sin.rsz()) ;
    }
    else
    {
        _remote_sin.sin_port        = htons (port);
        _remote_sin.sin_family      = AF_INET;
        if(ip)
            _remote_sin.sin_addr.s_addr = inet_addr(ip); // direct
        else
            _remote_sin.sin_addr.s_addr = inet_addr("255.255.255.255");

        ////printf("<-to %s : %d\n", IP2STR(_remote_sin.sin_addr.s_addr), htons(_remote_sin.sin_port));

        snd = ::sendto(_thesock, (char*)buff, length, 0,
                       (struct sockaddr  *) &_remote_sin,
                       _remote_sin.rsz()) ;
    }
    if(-1 == snd)
    {
        _error = errno;
    }
    return snd;
}

//-----------------------------------------------------------------------------
int  udp_sock::send(const char* buff, const int length, const SADDR_46& rsin)
{
    return send((const unsigned char*) buff, length, rsin);
}

//-----------------------------------------------------------------------------
int  udp_sock::send(const unsigned char* buff, const int length, const SADDR_46& rsin)
{
    int snd ;
    _error = 0;
    if(_connected && rsin == _remote_sin)
    {
        snd = ::send(_thesock,(char *)buff, length, 0);
    }
    else
    {
        ////printf("<-to %s : %d\n", IP2STR(rsin.sin_addr.s_addr), htons(rsin.sin_port));
        snd = ::sendto(_thesock, (char*)buff, length, 0,
                       (struct sockaddr  *) &rsin,
                       rsin.rsz()) ;
    }
    if(-1 == snd)
    {
        _error = errno;
    }
    return snd;
}



//-----------------------------------------------------------------------------
int  udp_sock::receive(char* buff, int length, SADDR_46& rsin)
{
    return receive((unsigned char*) buff, length, rsin);
}

//-----------------------------------------------------------------------------
int  udp_sock::receive(unsigned char* buff, int length, SADDR_46& rsin)
{
    int         rcv;
    SADDR_46    sin = rsin;

    if(_connected)
    {
        rcv = ::recv(_thesock,(char *)buff, length, 0);
    }
    else
    {
        socklen_t iRecvLen=(socklen_t)sin.rsz();
        rcv =  (int)::recvfrom (_thesock,
                                (char*)buff,
                                length,
                                0,
                                (struct sockaddr  *) &sin,
                                &iRecvLen);
    }
    if(rcv==-1)
    {
        _error = errno;
    }
    rsin=sin;
    return rcv;
}

//-----------------------------------------------------------------------------
int  udp_sock::receive(char* buff, int length, int port, const char* ip)
{
    if(_bind)
    {
        socklen_t iRecvLen=(socklen_t)_remote_sin.rsz();
        int rcv =  (int)::recvfrom (_thesock,
                                    (char*)buff,
                                    length,
                                    0,
                                    (struct sockaddr  *) &_remote_sin,
                                    &iRecvLen);
        return rcv;
    }
    return receive((unsigned char*)buff, length, port, ip);
}

//-----------------------------------------------------------------------------
int  udp_sock::receive(unsigned char* buff, int length, int port, const char* ip)
{
    _error  = 0;
    int rcv ;
    if(_connected)
    {
        rcv = ::recv(_thesock,(char *)buff, length, 0);
    }
    else
    {
        if(ip)
        {
            _remote_sin.sin_addr.s_addr = inet_addr(ip);
            _remote_sin.sin_port        = htons (port);
            _remote_sin.sin_family      = AF_INET;
            socklen_t iRecvLen          = _remote_sin.rsz();
            rcv =  (int)recvfrom (_thesock,
                                  (char*)buff, length,
                                  0,
                                  (struct sockaddr  *) &_remote_sin,
                                  &iRecvLen);

        }
        else
        {
            memset(&_remote_sin,0,sizeof(_remote_sin));
            socklen_t iRecvLen   = _remote_sin.rsz();
            rcv =  (int)recvfrom (_thesock, (char*)buff, length,
                                  0,
                                  (struct sockaddr  *) &_remote_sin,
                                  &iRecvLen);

        }
    }

    if(rcv==-1)
    {
        _error = errno;
    }
    return rcv;
}

//-----------------------------------------------------------------------------
char* udp_sock::ssock_addrip()
{
    ::strcpy(_sip, inet_ntoa((struct in_addr)_remote_sin.sin_addr));
    return _sip;
}


int  udp_sock::set_rsin(const char* sip, int port)
{
    _connected      = 0;

    _remote_sin.sin_family		= AF_INET;
    _remote_sin.sin_addr.s_addr	= inet_addr(sip);
    _remote_sin.sin_port		= htons(port);
    _bind = true;
    return 0;
}


int  udp_sock::connect(const char* sip, int port, CancelCB cbCall, void* pUser)
{
    int             err;
    fd_set          fdWr;
    timeval         tv = {1, 0};
    _connected      = 0;

    _remote_sin.sin_family		= AF_INET;
    _remote_sin.sin_addr.s_addr	= inet_addr(sip);
    _remote_sin.sin_port		= htons(port);
    memset(&_remote_sin.sin_zero, 0, sizeof(_remote_sin.sin_zero));

    // non blocking node couse we can cancel it by Cancel f_call
    bio_unblock     bl(this);
    time_t          ti = time(0);
    err = ::connect(_thesock, (const struct sockaddr*)&_remote_sin, _remote_sin.rsz());
    if(0==cbCall)
        cbCall = sock::DefCBCall;

    while(cbCall(pUser, time(0)-ti))
    {
        FD_ZERO(&fdWr);
        FD_SET((unsigned int)_thesock, &fdWr);
        int nfds = (int)_thesock+1;

        err = ::select(nfds,0,&fdWr,0,&tv);
        if(err > 0)
        {
            if(FD_ISSET(_thesock, &fdWr))
            {
                _connected = 1;
                return 0;   // no error
            }
        }
        usleep(10000);
    }
    destroy();
    return -1;
}


//-----------------------------------------------------------------------------
/*
// ip range 224.0.0.0 to 238.255.255.255 as
SOCKET udp_group_sock::create(int opt)
{
    _error = 0;
    ::memset(&_mcastGrp,0,sizeof(_mcastGrp));
    _groupmember = FALSE;

    // from server
    _local_sin.sin_family      = AF_INET;
    _local_sin.sin_port        = htons (0);
    _local_sin.sin_addr.s_addr = htonl (INADDR_ANY);

    if (::bind (_thesock, (struct sockaddr*) &_local_sin,
              sizeof (_local_sin)) == -1)
    {
        _error =  errno;
        destroy();
        return -1;
    }
    int iOptVal = opt;  // time to live (in the net)
    if (setsockopt (_thesock, IPPROTO_IP, IP_MULTICAST_TTL,
                    (char  *)&iOptVal, sizeof (int)) == -1)
    {
        _error =  errno;
        destroy();
        return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
int udp_group_sock::send(const unsigned char* buff, int length, int port, const char* ipGrp)
{
    int snd;

    _error = 0;
    _remote_sin.sin_family = AF_INET;
    _remote_sin.sin_port = htons (port);
    _remote_sin.sin_addr.s_addr = inet_addr(ipGrp);
    snd = ::sendto (_thesock, (char*)buff, length, 0,
                    (struct sockaddr  *) &_remote_sin, sizeof (_remote_sin)) ;
    if(snd < 0)
    {
        _error = errno;
    }
    return snd;
}

//-----------------------------------------------------------------------------
int udp_group_sock::receive(unsigned char* pbuff, int length, int port, const char* ipGrp)
{
    _error = 0;
    int iRecvLen = sizeof (_remote_sin);
    int i =  (int)recvfrom (_thesock, (char*)pbuff, length, 0,
                            (struct sockaddr  *) &_remote_sin, &iRecvLen);
    if(i==-1)
    {
        _error = errno;
    }
    return i;
}

//-----------------------------------------------------------------------------
bool    udp_group_sock::destroy()
{
    if(_groupmember)
    {
        _error = ::setsockopt (_thesock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                              (char  *)&_mcastGrp, sizeof (_mcastGrp)) ;
    }
    udp_sock::destroy();
}

//-----------------------------------------------------------------------------
int udp_group_sock::join(const char* ipGrp, int port)
{
    if(_thesock != -1)
        return -1;

    _error = 0;
    _local_sin.sin_family = AF_INET;
    _local_sin.sin_port = htons (port);
    _local_sin.sin_addr.s_addr = htonl (INADDR_ANY);
    int one = 1;
    ::setsockopt(_thesock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    int bnd = ::bind (_thesock, (struct sockaddr  *) &_local_sin, sizeof (_local_sin));
    if (bnd == -1)
    {
        _error =  errno;
        destroy();
        return bnd;
    }

    //join the multicast group
    _mcastGrp.imr_multiaddr.s_addr = inet_addr (ipGrp);
    _mcastGrp.imr_interface.s_addr = INADDR_ANY;
    bnd = ::setsockopt (_thesock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char  *)&_mcastGrp, sizeof (_mcastGrp)) ;
    if (bnd == -1)
    {
        _error =  errno;
        destroy();
        return bnd;
    }
    _groupmember = TRUE;
    return 0;
}
*/



Ip2str::Ip2str(const SADDR_46& sa){
    if(sa.sin_addr.s_addr)
    {
        u_int32_t dw = _IP(sa);
        ::sprintf( _s,"%u.%u.%u.%u:%d",//%04X:%04X:%04X:%04X:%04X:%04X",
                   (int) ((dw >> 24) & 0xFF),
                   (int) ((dw >>16) & 0xFF),
                   (int) ((dw >>8) & 0xFF),
                   (int) (dw  & 0xFF),
                   htons(sa.sin_port));
    }else
    {
        ::strcpy(_s,"0.0.0.0:0");
    }
}

Ip2str::Ip2str(const u_int32_t dw){
    ::sprintf( _s,"%u.%u.%u.%u:0",//%04X:%04X:%04X:%04X:%04X:%04X",
               (int) ((dw >> 24) & 0xFF),
               (int) ((dw >>16) & 0xFF),
               (int) ((dw >>8) & 0xFF),
               (int) (dw  & 0xFF));
}

std::string macaddr()
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    char out[32];

    sprintf(out,"%012X",rand());
    std::string ret=out;

    strcpy(s.ifr_name, "eth0");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        int i;
        ret.clear();
        for (i = 0; i < 6; ++i)
        {
            sprintf(out, "%02x", (unsigned char) s.ifr_addr.sa_data[i]);
            ret+=out;
        }
        close(fd);
        return  ret;
    }

    strcpy(s.ifr_name, "eno1");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        int i;
        ret.clear();
        for (i = 0; i < 6; ++i)
        {
            sprintf(out, "%02x", (unsigned char) s.ifr_addr.sa_data[i]);
            ret+=out;
        }
        close(fd);
        return  ret;
    }

    strcpy(s.ifr_name, "wlan0");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        int i;
        ret.clear();
        for (i = 0; i < 6; ++i)
        {
            sprintf(out, "%02x", (unsigned char) s.ifr_addr.sa_data[i]);
            ret+=out;
        }
        close(fd);
        return ret;
    }

    strcpy(s.ifr_name, "wlan1");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        int i;
        for (i = 0; i < 6; ++i)
        {
            ret.clear();
            sprintf(out, "%02x", (unsigned char) s.ifr_addr.sa_data[i]);
            ret+=out;
        }
        close(fd);
        return ret;
    }
    return ret;
}


