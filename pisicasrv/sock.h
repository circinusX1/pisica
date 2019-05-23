/**
# Copyright (C) 2007-2015 s(marrius9876@gmail.com)
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

#ifndef __SOCKX_H__
#define __SOCKX_H__

#include "osthread.h"
#include <sys/socket.h>
#include <resolv.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <math.h>
using namespace std;

#ifdef _WIN32
#pragma warning (disable: 4267)
#pragma warning (disable: 4244)
#endif //_WIN32

#ifndef _WIN32
    typedef int SOCKET;
#else
    typedef int socklen_t;
#endif //

//---------------------------------------------------------------------------------------
#define MAKE_IP(a_,b_,c_,d_) (unsigned long)( (a_ <<24) | (b_<<16) | (c_<<8) | d_)

//---------------------------------------------------------------------------------------
typedef bool (*CancelCB)(void* pVoid, unsigned long time);

#ifdef WIN32
    #define EINPROGRESS         WSAEINPROGRESS
    #define WOULDBLOCK          WSAEWOULDBLOCK
#else
    #define WOULDBLOCK          EINPROGRESS
#endif //

//---------------------------------------------------------------------------------------
class tcp_sock;

typedef struct sockaddr_in  SA_46;
struct bio_unblock;

//fake till migrate to 4 and 6
/*
    _remote_sin.sin_family		= AF_INET;
    _remote_sin.sin_addr.s_addr	= htonl(ip.ip4());
    _remote_sin.sin_port		= htons(port);
*/
//---------------------------------------------------------------------------------------
#ifndef IPV6

#define _IPR(asin)   (asin.sin_addr.s_addr)
#define _IP(asin)    ::htonl(asin.sin_addr.s_addr)
#define _PORT(asin)  ::htons(asin.sin_port)

struct SADDR_46 ;
class Ip2str
{
public:
    Ip2str(const SADDR_46& sa);
    Ip2str(const u_int32_t dw);
    operator const char*()const{return _s;}
private:
   char _s[128];
};


#define IP2STR(dwip_)   (const char*)Ip2str(dwip_)

//---------------------------------------------------------------------------------------
// holds in network order
//---------------------------------------------------------------------------------------
struct SADDR_46 : public SA_46
{
    SADDR_46(){clear();}
    void clear(){memset(this,0, sizeof(*this)); sin_family=AF_INET; _mask=0xFFFFFFFF;_sip[0]='*';}

    SADDR_46(u_int32_t r, uint16_t port=0, uint32_t mask=0xFFFFFFFF){
        sin_addr.s_addr = htonl(r);
        sin_port = htons(port);
        sin_family=AF_INET;
        _mask = mask;
       ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));
    }
    SADDR_46(const SADDR_46& r){::memcpy(this, &r, sizeof(*this));}

    SADDR_46(const char* r, uint16_t port=0, uint32_t mask=0xFFFFFFFF){

        from_string(r,  port, mask);
       ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));

    }
    SADDR_46(const SA_46& sa){::memcpy(this, &sa, sizeof(*this));}
    SADDR_46& operator=(const SADDR_46& r){
        ::memcpy(this, &r, sizeof(*this));
        ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));
        return *this;
    }
    void commit(){::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));}
    inline int rsz()const{return sizeof(SA_46);}
    void addr_htonl(){sin_addr.s_addr = htonl(sin_addr.s_addr);}
    void port_htons(){sin_port = htons(sin_port);}
    void from_string( const char* r, uint16_t port=0, uint32_t mask=0xFFFFFFFF)
    {
        char l[128]; ::strcpy(l,r);
        char* pp = ::strstr((char*)l,":");
        if(pp){
            *pp=0;
            port=::atoi(pp+1);
        }

        char* pm = 0;
        if((pm = (char*)strchr(l,'/')) != 0) //has range ip
        {
            *pm++=0;
            sin_addr.s_addr = inet_addr(l);
            sin_port = htons(port);
            sin_family=AF_INET;
            int msk = pow(2,atoi(pm))-1;
            _mask=htonl(~msk);
        }
        else
        {
            sin_addr.s_addr = inet_addr(l);
            sin_port = htons(port);
            sin_family=AF_INET;
            _mask = mask;
        }
       ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));

    }

    bool compare_range_ip(const SADDR_46& r)const
    {
        int32_t m = _mask & r._mask;
        return ((sin_addr.s_addr&m) == (r.sin_addr.s_addr&m));
    }
    bool operator != (const SADDR_46& r)const
    {
        int32_t m = _mask & r._mask;
        if(sin_port == 0 && r.sin_port == 0)
        {
            return (sin_addr.s_addr&m) != (r.sin_addr.s_addr&m);
        }
        u_int64_t bigint = ((((u_int64_t)(sin_addr.s_addr&m)<<32)) | (u_int64_t)sin_port);
        u_int64_t bigintr = ((((u_int64_t)(r.sin_addr.s_addr&m))<<32) | (u_int64_t)r.sin_port);
        return bigint != bigintr;
    }
    bool isequal(const SADDR_46& r, bool porttoo=false)const
    {
        int32_t m = _mask & r._mask;
        if(!porttoo)
        {
             return ((sin_addr.s_addr&m) == (r.sin_addr.s_addr&m));
        }
        u_int64_t bigint = ((((u_int64_t)sin_addr.s_addr&m)<<32) | (u_int64_t)sin_port);
        u_int64_t bigintr = ((((u_int64_t)r.sin_addr.s_addr&m)<<32) | (u_int64_t)r.sin_port);
        return bigint == bigintr;

    }
    bool operator == (const SADDR_46& r)const
    {
        int32_t m = _mask & r._mask;
        if(sin_port == 0 || r.sin_port == 0)
        {

             return ((sin_addr.s_addr&m) == (r.sin_addr.s_addr&m));
        }
        u_int64_t bigint = ((((u_int64_t)sin_addr.s_addr&m)<<32) | (u_int64_t)sin_port);
        u_int64_t bigintr = ((((u_int64_t)r.sin_addr.s_addr&m)<<32) | (u_int64_t)r.sin_port);
        return bigint == bigintr;
    }
    bool operator < (const SADDR_46& r)const{
        int32_t m = _mask & r._mask;
        if(sin_port == 0 || r.sin_port==0)
        {
            return (sin_addr.s_addr&m) < (r.sin_addr.s_addr&m);
        }
        u_int64_t bigint = ((((u_int64_t)sin_addr.s_addr&m)<<32) | (u_int64_t)sin_port);
        u_int64_t bigintr = ((((u_int64_t)r.sin_addr.s_addr&m)<<32) | (u_int64_t)r.sin_port);
        return bigint < bigintr;
    }
    bool operator > (const SADDR_46& r)const{
        int32_t m = _mask & r._mask;
        if(sin_port == 0 || r.sin_port==0)
        {
            return (sin_addr.s_addr & m) > (r.sin_addr.s_addr & m);
        }
        u_int64_t bigint = ((((u_int64_t)sin_addr.s_addr&m)<<32) | (u_int64_t)sin_port);
        u_int64_t bigintr = ((((u_int64_t)r.sin_addr.s_addr&m)<<32) | (u_int64_t)r.sin_port);
        return bigint > bigintr;
    }
    void set(u_int32_t r,  uint16_t port=0){
            sin_addr.s_addr = htonl(r);
            sin_port = htons(port);
            ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));
        }
    void set_port(uint16_t port){sin_port = htons(port);}
    u_int32_t ip4()const{return htonl(sin_addr.s_addr);}    //ret in normal order
    u_int32_t port()const{return htons(sin_port);}          //ret in normal order
    const char* c_str()const{return _sip;};
    operator const char*()const{return _sip;}
    bool empty()const{return sin_addr.s_addr==0;}

    const SADDR_46& operator>>(ofstream& o) const
    {
        char fmt[64];
        if(_mask!=0xFFFFFFFF)
        {
            int bits=0;
            uint32_t m = ~_mask;
            while((m&(0x1)) == 0x1)
            {
                m>>=1;
                ++bits;
            }
            sprintf(fmt, "%d/%d:%d\n", ip4(), bits , port());
        }
        else
        {
            sprintf(fmt, "%d:%d\n", ip4(), port());
        }
        o.write(fmt, strlen(fmt));
        return *this;
    }

    SADDR_46& operator<<(ifstream& i)
    {
        std::string s;
        std::getline(i, s);
        int uip=0, port=0;
        if(s.find('/'))//has range of ip's
        {
            int bits;
            _mask=0xFFFFFFFF;
            sscanf(s.c_str(),"%d/%d:%d",&uip, &bits, &port);
            int msk = pow(2,bits)-1;
            _mask=htonl(~msk);

        }
        else
        {
            _mask=0xFFFFFFFF;
            sscanf(s.c_str(),"%d:%d",&uip, &port);
        }
        if(uip)
            set(uip, (uint16_t)port);
        ::strcpy(_sip,inet_ntoa((struct in_addr)this->sin_addr));

        return *this;
    }
    uint32_t _mask;
    char     _sip[32];
};

#else // to do
union SADDR_46  {
    struct sockaddr sa;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
};
#endif



//---------------------------------------------------------------------------------------
class sock
{
public:
    friend struct bio_unblock;
    static      bool DefCBCall(void*,unsigned long);
    static void Init();
    static void Uninit();
    static char*  GetLocalIP(const char* reject);
    static bool CTime(void* pT, unsigned long time);
    static SADDR_46 dnsgetip(const char* sip, char* out=0, int port=0);
    static SADDR_46 sip2ip(const char* sip, uint16_t port=0);
    static bool dnsgetname(u_int32_t uip, char* out);
    static bool dnsgetnameinfo(const SADDR_46& uip, char* out);
    sock();
    virtual ~sock();
    virtual SOCKET  create(int port, int opt=0, const char* inetaddr=0);
    virtual SOCKET  create(const SADDR_46& r,int opt=0);
    virtual bool    destroy(bool be=true);
    virtual int     send(const char* buff, const int length, int port=0, const char* ip=0  )=0;
    virtual int     send(const char* buff, const int length, const  SADDR_46& rsin)=0;
    virtual int     send(const unsigned char* buff, const int length, int port=0, const char* ip=0  )=0;
    virtual int     send(const unsigned char* buff, const int length, const  SADDR_46& rsin)=0;
    virtual int     receive(unsigned char* buff, int length, int port=0, const char* ip=0  )=0;
    virtual int     receive(unsigned char* buff, int length,  SADDR_46& rsin)=0;
    virtual int     select_receive(unsigned char* buff, int length, int toutms, int wait=0);
    virtual int     receive(char* buff, int length, int port=0, const char* ip=0  )=0;
    virtual int     receive(char* buff, int length,  SADDR_46& rsin)=0;
    virtual int     select_receive(char* buff, int length, int toutms, int wait=0);
    int             detach(){int s = _thesock; _thesock=-1; return s;}
    void            attach(int s);
    int             set_blocking(const unsigned long block);
    int             set_option(int options, int getJson);
    int             get_option(int options);
    SOCKET&         socket() {return _thesock;}
    virtual bool    isopen()const{return (int)_thesock > 0;}
    int             error()const{return _error;}
    int             is_blocking(){return _blocking;}
    void            pre_set(int sb, int rb){_buffers[0]=sb;_buffers[1]=rb;}
    void            reset(){_io = 0;};
    int&            set(){return _io;};
    SADDR_46&        ip46(){return _ipfixit;}
    void            set(int mask){_io|=mask;};
    const SADDR_46& Rsin()const{return _remote_sin;}
    const SADDR_46& Lsin()const{return _local_sin;}

protected:
    SOCKET          _thesock;
    int             _io;
    int             _error;
    int             _blocking;
    int             _buffers[2];
    SADDR_46        _ipfixit;
    SADDR_46	    _local_sin;	        // source
	SADDR_46	    _remote_sin;          // dest
    static  unsigned long   _tout;

};

//---------------------------------------------------------------------------------------
class tcp_srv_sock;
class tcp_sock : public sock
{
public:
    friend class tcp_srv_sock;
    tcp_sock();
    virtual ~tcp_sock();
    virtual SOCKET  create(int port, int opt=0, const char* inetaddr=0);
    virtual SOCKET  create(const SADDR_46& r, int opt=0);
    virtual bool    isopen()const;
    virtual int     send(const char* buff, const int length, int port=0, const char* ip=0  );
    virtual int     send(const char* buff, const int length, const  SADDR_46& rsin){UNUS(rsin); return send(buff, length);};
    virtual int     send(const unsigned char* buff, const int length, int port=0, const char* ip=0  );
    virtual int     send(const unsigned char* buff, const int length, const  SADDR_46& rsin){UNUS(rsin) ;return send(buff, length);}
    virtual int     receive(unsigned char* buff, int length, int port=0, const char* ip=0  );
    virtual int     receive(unsigned char* buff, int length,  SADDR_46& rsin){UNUS(rsin); return receive(buff,length);}
    virtual int     receive(char* buff, int length, int port=0, const char* ip=0  );
    virtual int     receive(char* buff, int length,  SADDR_46& rsin){UNUS(rsin); return receive(buff,length);}
    virtual int     sendall(const unsigned char* buff,  int length, int tout=8000);
    virtual int     sendall(const char* buff, int length, int tout=8000){return sendall((unsigned char*)buff, length, tout);}
    virtual bool    destroy(bool be=true);
    int             receiveall(unsigned char* buff, const int length);

    char*           ssock_addrip();
    char*           getsocketaddr_str(char* pAddr)const;
    const SADDR_46&       getsocketaddr()const;
    int             getsocketport()const;
    int             listen(int maxpending=64);

protected:

    char    _sip[32];
};


//---------------------------------------------------------------------------------------
class tcp_cli_sock;
class tcp_srv_sock : public tcp_sock
{
    int n_port;
public:
    int port()const{return n_port;}
    tcp_srv_sock();
    virtual ~tcp_srv_sock();
    virtual bool    destroy(bool be=true);
    SOCKET     accept(tcp_cli_sock& cliSock);

    virtual SOCKET   create(int port, int opt=0, const char* inetaddr=0);
    virtual SOCKET  create(const SADDR_46& r, int opt=0);
};

//---------------------------------------------------------------------------------------
class tcp_cli_sock : public tcp_sock
{
public:
    tcp_cli_sock();
    tcp_cli_sock(const tcp_cli_sock& s);
    tcp_cli_sock& operator=(const tcp_cli_sock& s);
    virtual         ~tcp_cli_sock();
    virtual SOCKET  create(int port, int opt=0, const char* inetaddr=0);
    virtual SOCKET  create(const SADDR_46& r, int opt=0);
    virtual int     raw_connect(const SADDR_46& uip4, int tout=0);
    virtual int     raw_connect(u_int32_t ip4,  int port);
	virtual int     raw_connect_sin();
    virtual bool    isopen()const;
    virtual bool    destroy(bool be=true);
    int             try_connect(const char* sip, int port);
	void            raw_sethost(const SADDR_46& uip4);
    int             openconnection(const char* sip, int port);
    int             connect(const char* sip, int port, CancelCB cbCall=sock::DefCBCall, void* pUser=0);
    int             i4connect(const SADDR_46& ip, CancelCB cbCall=sock::DefCBCall, void* pUser=0);
    int             s4connect(const char* sip, int port, CancelCB cbCall=sock::DefCBCall, void* pUser=0);
    bool            is_really_connected();
    bool            check_connection()const {return _connected;};
    struct hostent* gethostent(){return _hostent;}

    int             isconnecting(){
        return _connecting;
    }
    virtual void            setconnected(){
        _connecting = 0;
        _connected = 1;
    }

protected:
     hostent  *_hostent;
    int  _connecting;
    int  _connected;
};

//---------------------------------------------------------------------------------------
class udp_sock : public sock
{
public:
    udp_sock():_connected(0),_bind(0){}
    virtual ~udp_sock(){destroy(false);}
    virtual bool    destroy(bool be=true){bool b = sock::destroy(be);_connected=0;return b;};
    virtual SOCKET  create(int port, int opt=0, const char* inetaddr=0);
    virtual SOCKET  create(const SADDR_46& r, int opt=0);
    virtual int     send(const char* buff, const int length, int port=0, const char* ip=0  );
    virtual int     send(const char* buff, const int length, const  SADDR_46& rsin);
    virtual int     send(const unsigned char* buff, const int length, int port=0, const char* ip=0  );
    virtual int     send(const unsigned char* buff, const int length, const  SADDR_46& rsin);
    virtual int     receive(unsigned char* buff, int length, int port=0, const char* ip=0  );
    virtual int     receive(unsigned char* buff, int length,  SADDR_46& rsin);
    virtual int     receive(char* buff, int length, int port=0, const char* ip=0  );
    virtual int     receive(char* buff, int length,  SADDR_46& rsin);
    void            SetRsin(const  SADDR_46& in){::memcpy(&_remote_sin, &in, sizeof( SADDR_46));}
    int             connect(const char* sip, int port, CancelCB cbCall=sock::DefCBCall, void* pUser=0);
    int             set_rsin(const char* sip, int port);
    int             bind(const char* ip=0, int port=0);
    SADDR_46&          remote(){return _remote_sin;}
    void            remote(SADDR_46& s){memcpy(&_remote_sin,&s,sizeof(s));}
    char*           ssock_addrip();

protected:
    bool    _connected;
    bool    _bind;
    int     _transport;
    char    _sip[128];
};


//---------------------------------------------------------------------------------------
class udp_group_sock : public udp_sock
{
/*
    struct ip_mreq    _mcastGrp;
    bool               _groupmember;

public:
    udp_group_sock(int opt=0){};
    virtual ~udp_group_sock(){destroy();};
    virtual int  create(int opt=0);
    int     join(const char* ipGrp, int port);

    virtual int     send(unsigned char* buff, int length, int port=0, const char* ip=0  );
    virtual int     receive(unsigned char* buff, int length, int port=0, const char* ip=0  );
    virtual bool    destroy();
    */
};

#define IS_SOCKK_ERR(err_)  err_ == WSAECONNRESET   ||\
                            err_ == WSAECONNABORTED ||\
                            err_ == WSAESHUTDOWN    ||\
                            err_ == WSAETIMEDOUT    ||\
                            err_ == WSAECONNREFUSED ||\
                            err_ == WSAEOPNOTSUPP   ||\
                            err_ == WSAENETDOWN     ||\
                            err_ == -1




class WsaInit
{
public:
    WsaInit()
    {
        sock::Init();
    }
    ~WsaInit()
    {
        sock::Uninit();
    }
};


struct bio_unblock
{
    sock* _sk;
    int   _bl;
    bio_unblock(sock* sock, int bl=0);
    ~bio_unblock();
};

std::string macaddr();

#endif // !
