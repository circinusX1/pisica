
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
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
    ::sleep(3);
    OsThread::stop_thread();
}

void frameclient::thread_main()
{
    size_t      tinq;
    time_t      whenc=time(0)+5;
    uint32_t    hdrsz;
    tcp_cli_sock cli;

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
                cli.destroy();
                ::msleep(256);
                std::cout << "try cannot connect \n";
                goto FLUSH;
            }
            if(cli.is_really_connected())
            {
                std::cout << "connected to: " <<  _url->host <<":" <<_url->port << "\n";
            }
            else
            {
                ++_connectfail;
                cli.destroy();
                ::msleep(256);
                std::cout << "try connect failed \n";
            }
        }
        _connectfail=0;
        if(cli.is_really_connected())
        {
            unsigned char incoming[256];
            char     md5sig[33] = {0};

            // clear text mac + random
            std::string sr = _mac + "-";
            sr += std::to_string(rand()) +
                  std::to_string(rand()) +
                  std::to_string(rand());
            ::mg_md5(md5sig,(sr+_auth).c_str(),nullptr);
            std::string tokenex = md5sig;

            if(cli.sendall(sr.c_str(),sr.length()) != (int)sr.length())
            {
                cli.destroy();
                std::cout << "send all authorizarion failed \n";
                goto FLUSH;
            }

            //
            // make sure we sent 256 back
            //
            int bytes = cli.receive(incoming, sizeof(incoming));
            if(bytes>0)
            {
                char token[128];

                std::cout << ">" << incoming << "\n";
                incoming[bytes] = 0;
                std::string inc = (char*)incoming;
                if(tokenex == inc)
                {
                    if(cli.sendall((const unsigned char*)&_cfg, sizeof(_cfg)) != sizeof(_cfg))
                    {
                        cli.destroy();
                        std::cout << "send all confirmation failed \n";
                        goto FLUSH;
                    }
                    _stream(cli);
                }
                else
                {
                    std::cout << "password confirmation failed \n";
                    cli.destroy();
                }
            }
        }
        else
        {
            std::cout << "is not connected \n";
            cli.destroy();
            ::sleep(1);
        }
FLUSH:
        frame* pf = _pq->deque();
        if(pf)
        {
            delete pf;
        }
        ::msleep(20);
    }
    cli.destroy();
}


void frameclient::_stream(tcp_cli_sock& cli)
{
    fd_set  rset;

    ::msleep(258);

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
                if(bytes >= sizeof(_fconf))
                {
                    _cbn(&_fconf);
                }
                if(bytes==0)
                {
                    cli.destroy();
                    std::cout << "remote closed connection\n";
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
                std::cout << "Pushes:" << _bps/5 << "\n";
                _bps=0;
                _now=_lastfrmtime;
            }
            delete pf; pf=nullptr;
            _noframe = 0;
        }
        else {
            if(++_noframe > 100)
            {
                std::cout << "no streaming. closing connection\n";
                cli.destroy();
            }
        }
        if(_fconf.motion==0 ||
           _fconf.lapse==0 ||
           _fconf.client==0)
        {
            cli.destroy();
            break;
        }

    }
}


