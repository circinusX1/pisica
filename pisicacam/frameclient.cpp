
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "config.h"
#include "frameclient.h"
#include "vigenere.h"
#include "md5.h"

#define MAX_CONNECTS 65535
#define RECON_TOUT   8

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
    time_t      whenc=time(0)+5;
    tcp_cli_sock cli;
    bool        streamnow=false;

    srand(time(0));
    while(__alive && !this->is_stopped())
    {
        if(time(0)>whenc && !cli.isopen())
        {
            whenc=time(0) + RECON_TOUT;       // every 5 seconds
            COUT_("RECONNECTING ");
            cli.destroy();
            cli.create(_url->port);
            cli.set_blocking(1);

            int val = true;
            ::setsockopt(cli.socket(), IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
            if(cli.try_connect(_url->host,_url->port,4000)==0)
            {
                ++_connectfail;
                COUT_( "TRY CANNOT CONNECT ");
                goto FLUSH;
            }
            if(cli.is_really_connected())
            {
                COUT_( "FRM CONNECTED TO: " <<  _url->host <<":" <<_url->port );
            }
            else
            {
                ++_connectfail;
                COUT_( "TRY CONNECT FAILED ");
                goto FLUSH;
            }
        }
        _connectfail=0;
        if(cli.is_really_connected())
        {
            struct          config cloco;

            COUT_("REALLY CONNECTED ");
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
            COUT_("SENDING " << sizeof(cloco));
            if(cli.sendall((const unsigned char*)&cloco, sizeof(cloco)) != sizeof(cloco))
            {
                COUT_( "SEND ALL AUTHORIZARION FAILED ");
                goto FLUSH;
            }
            ::msleep(0xFF);
            // server decrypt with cam pass an send back the config
            int sent = cloco.rndm;
#ifdef DEBUG
            if(cli.select_receive((unsigned char*)&cloco, sizeof(cloco), 60000) != sizeof(cloco))
#else
            if(cli.select_receive((unsigned char*)&cloco, sizeof(cloco), 2000) != sizeof(cloco))
#endif
            {
                COUT_( "RECEIVE FAILED");
                goto FLUSH;
            }
            if(cloco.rndm != sent)
            {
                COUT_( "RECEIVE ALL AUTHORIZARION FAILED ");
                goto FLUSH;
            }
            LOCK_(_pm, _cfg = cloco);
            streamnow=true;
        }
        if(streamnow)
            _stream(cli);
FLUSH:
        if(cli.isopen())
        {
            COUT_("DESTROING CONNECTION FLUSH ");
            cli.destroy();
        }
        frame* pf = _pq->deque();
        if(pf)
            delete pf;
        ::msleep(4);
        streamnow=false;
    }
    COUT_("DESTROING CONNECTION EXIT");
    cli.destroy();
}


void frameclient::_stream(tcp_cli_sock& cli)
{
    fd_set  rset;
    int     somtime;

    ::msleep(258);
    if(!cli.is_really_connected()){
        COUT_( "STREAM WHILE OFFLINE ");
        cli.destroy();
        return;
    }
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
                    COUT_( "REMOTE REQUESTED CONFIG");
                    _cbn(&_fconf);
                }
                if(bytes==0)
                {
                    cli.destroy();
                    COUT_( "REMOTE CLOSED CONNECTION");
                    break;
                }
                if(_fconf.motion+_fconf.timelapse+_fconf.webclienton==0)
                {
                    cli.destroy();
                    COUT_( "REMOTE DOES NOT WANT STREAMING");
                    break;
                }
            }
        }
        frame* pf = _pq->deque();
        if(pf)
        {
            if((++somtime & 0x1F)==0x1F){
                std::cout <<(".");
                std::cout.flush();
            }
            size_t sent = cli.sendall(pf->buffer(), pf->length());
            if(sent != pf->length())
            {
                COUT_( "REMOTE CLOSED " <<  _url->host <<":" <<_url->port );
                cli.destroy();
                break;
            }
            if(sent>0)
                _bps+=sent;
            _lastfrmtime=time(0);
            if(_lastfrmtime>(_now+5))
            {
                COUT_( "PUSHES:" << _bps/5 );
                _bps=0;
                _now=_lastfrmtime;
            }
            delete pf; pf=nullptr;
            _noframe = MAX_NO_FRAMES;
        }
        else
        {
            if(--_noframe <= 0)
            {
                COUT_( "NO FRAMES STREAMING. CLOSING CONNECTION");
                cli.destroy();
            }
        }
    }
}



