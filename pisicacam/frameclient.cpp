
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "config.h"
#include "frameclient.h"
#include "vigenere.h"
#include "md5.h"

#define MAX_CONNECTS 65535

frameclient::frameclient(cbnoty ns,
                         streamq* pq,
                         const urlinfo* url,
                         const char* auth):
    _pq(pq),
    _cbn(ns),
    _url(url),
    _auth(auth)
{
    _now = time(0);
    _bps = 0;
}

frameclient::~frameclient()
{
    stop_thread();
}

void frameclient::stop_thread()
{
    ::sleep(1);
    OsThread::stop_thread();
}

void frameclient::thread_main()
{
    size_t      tinq;
    time_t      whenc=time(0)+5;
    uint32_t    hdrsz;
    tcp_cli_sock cli;
    bool        streamnow=false;

    srand(time(0));
    while(__alive && !this->is_stopped())
    {
        if(time(0)>whenc && !cli.isopen())
        {
            whenc=time(0) + 5; // every 2 seconds
            cli.destroy();
            cli.create(_url->port);
            cli.set_blocking(1);

            int val = true;
            ::setsockopt(cli.socket(), IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
            if(cli.try_connect(_url->host,_url->port,4000)==0)
            {
                ++_connectfail;
                std::cout << "try cannot connect \n";
                goto FLUSH;
            }
            if(cli.is_really_connected())
            {
                std::cout << "FRM connected to: " <<  _url->host <<":" <<_url->port << "\n";
            }
            else
            {
                ++_connectfail;
                std::cout << "try connect failed \n";
                goto FLUSH;
            }
        }
        _connectfail=0;
        if(cli.is_really_connected())
        {
            struct          config cloco;
            char            md5sig[32] = {0};

            LOCK_(_pm, cloco=_cfg);
            // send encrypt(random, campass);

            ::strcpy(cloco.mac, _mac.c_str());
            // campass with srvpas
            ::strcpy(cloco.acl, xencrypt(_campsw,_srvpsw).c_str());
            ::strcpy(_cfg.mac, _mac.c_str());
            ::strcpy(cloco.acl1, xencrypt(_cfg.mac,_campsw).c_str());

            cloco.rndm = rand();
            std::string  srand = std::to_string(cloco.rndm);
            strcpy(cloco.acl2,xencrypt(srand, _campsw).c_str());

            if(cli.sendall((const unsigned char*)&cloco, sizeof(cloco)) != sizeof(cloco))
            {
                std::cout << "SEND ALL AUTHORIZARION FAILED \n";
                goto FLUSH;
            }
            ::msleep(0xFF);
            // server decrypt with cam pass an send back the config
            int sent = cloco.rndm;
            if(cli.select_receive((unsigned char*)&cloco, sizeof(cloco), 2000) != sizeof(cloco))
            {
                std::cout << "RECEIVE FAILED \n";
                goto FLUSH;
            }
            if(cloco.rndm != sent)
            {
                std::cout << "RECEIVE ALL AUTHORIZARION FAILED \n";
                goto FLUSH;
            }
            LOCK_(_pm, _cfg = cloco);
            streamnow=true;
        }
        if(streamnow)
            _stream(cli);
FLUSH:
        cli.destroy();
        frame* pf = _pq->deque();
        if(pf)
            delete pf;
        ::msleep(4);
        streamnow=false;
    }
    cli.destroy();
}


void frameclient::_stream(tcp_cli_sock& cli)
{
    fd_set  rset;

    ::msleep(258);

    std::cout << "STREAM WHILE OFFLINE \n";
    while(cli.is_really_connected())
    {
        FD_ZERO(&rset);
        FD_SET((unsigned int)cli.socket(), &rset);
        int     nfds = (int)cli.socket()+1;
        timeval tv = {0,8000};
        nfds = ::select(nfds,&rset,0,0,&tv);
        if(nfds>0)
        {
            if(FD_ISSET(cli.socket(), &rset))
            {
                int bytes = cli.receive((unsigned char*)&_fconf, sizeof(_fconf));
                if(bytes == sizeof(_fconf))
                {
                    _cbn(&_fconf);
                }
                if(bytes==0)
                {
                    cli.destroy();
                    std::cout << "REMOTE CLOSED CONNECTION\n";
                    break;
                }
                if(_fconf.motion+_fconf.lapse+_fconf.client==0)
                {
                    cli.destroy();
                    std::cout << "REMOTE DOES NOT WANT STREAMING\n";
                    break;
                }
            }
        }
        frame* pf = _pq->deque();
        if(pf)
        {
            size_t sent = cli.sendall(pf->buffer(), pf->length());
            if(sent != pf->length())
            {
                std::cout << "remote closed " <<  _url->host <<":" <<_url->port << "\n";
                cli.destroy();
                break;
            }
            if(sent>0)
                _bps+=sent;
            _lastfrmtime=time(0);
            if(_lastfrmtime>(_now+5))
            {
                std::cout << "PUSHES:" << _bps/5 << "\n";
                _bps=0;
                _now=_lastfrmtime;
            }
            delete pf; pf=nullptr;
            _noframe = 0;
        }
        else
        {
            if(++_noframe > 300)
            {
                std::cout << "NO FRAMES STREAMING. CLOSING CONNECTION\n";
                cli.destroy();
                _noframe = 0;
            }
        }
    }
}



