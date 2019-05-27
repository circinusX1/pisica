#include <unistd.h>
#include <fcntl.h>
#include "main.h"
#include "skcamsq.h"
#include "sks.h"
#include "skcam.h"
#include "skweb.h"
#include "skimg.h"
#include "sksrv.h"
#include "encrypt.h"
#include "request.h"
#include "vigenere.h"
#include "md5.h"

#define TTL_CLI 30

static char HDR[] = "HTTP/1.0 200 OK\r\n"
                    "Connection: close\r\n"
                    "Server: pisica/1.0\r\n"
                    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                    "Pragma: no-cache\r\n"
                    "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
                    "Content-type: text/html\r\n"
                    "Content-length: %d\r\n\r\n%s";

sksrv::sksrv(sks& p, skcamsq& q):_p(p),_q(q)
{
    p.link(this);
}

sksrv::~sksrv()
{
    _cli.destroy();
    _cam.destroy();
}

bool sksrv::spin(const char* auth, int cport, int cliport, pfn_cb cb)
{
    if(_cam.create(cport, SO_REUSEADDR, 0)>0)
    {
        _srvauth = auth;
        fcntl(_cam.socket(), F_SETFD, FD_CLOEXEC);
        _cam.set_blocking(0);
        if(_cam.listen(4)!=0)
        {
            _cam.destroy();
            return false;
        }
        LI("listening port "<< cport);
    }
    if(_cli.create(cliport, SO_REUSEADDR, 0)>0)
    {
        fcntl(_cli.socket(), F_SETFD, FD_CLOEXEC);
        _cli.set_blocking(0);
        if(_cli.listen(4)!=0)
        {
            _cli.destroy();
            return false;
        }
        LI("listening port "<< cliport);
    }
    fd_set r;
    time_t tl,now = time(0);
    while(__alive)
    {
        tl = time(0);

        _fd_check(r, _fd_set(r));
        cb();
        if(tl - now > 10)
        {
            AutoLock a(&_m);
            now = tl;
AGAIN:
            for(auto& a : _cliswait)
            {
                if(now-a.second.tstamp > TTL_CLI)
                {
                    _p.cliexpired(a.first);
                    _cliswait.erase(a.first);

                    goto AGAIN;
                }
            }
        }
        ::msleep(16);
    }
    return true;
}

int   sksrv::_fd_set(fd_set& rd)
{
    int ndfs = max(_cam.socket(),_cli.socket())+1;
    FD_ZERO(&rd);
    FD_SET(_cam.socket(), &rd);
    FD_SET(_cli.socket(), &rd);
    return ndfs;
}

void sksrv::_fd_check(fd_set& rd, int ndfs)
{
    timeval tv {0,0xFFFF};
    int     is = ::select(ndfs, &rd, 0, 0, &tv);
    if(is ==-1) {
        std::cerr << "socket select() critical " << "\n";
        __alive=false;
        return;
    }
    if(is>0)
    {
        try{
            if(FD_ISSET(_cli.socket(),&rd))
                _on_cli();
            if(FD_ISSET(_cam.socket(),&rd))
                _on_cam();
        }catch (skbase::STYPE& err){
            //already closed if not already queued to the pool
        }
    }
    return;
}

bool    sksrv::_on_cam()
{
    skbase s;

    if(_cam.accept(s)>0)
    {
        char     enough[256];
        char     replymd5[256];
        int      len = s.receive((uint8_t*)enough, sizeof(enough));
        if(len>0)
        {
            enough[len] = 0;
            std::string e = enough;
            // get mac
            const std::string tok = e; // ::dekr(e,_srvauth);
            size_t delim = tok.find("-");
            const std::string mac = tok.substr(0,delim);
            if(mac.length()==12)
            {
                std::string toenc = e + _srvauth;
                ::mg_md5(replymd5, toenc.c_str(), nullptr);
                if(s.sendall(replymd5,strlen(replymd5))==(int)strlen(replymd5))
                {
                    config c;
                    int l = s.receive((uint8_t*)&c,sizeof(c));
                    if(l==sizeof(c))
                    {
                        if(_p.has(mac))
                        {
                            LW("Conection refused: Already streaming from this MAC");
                            return false;
                        }

                        if(_regcamera(mac, c))
                        {
                            //s.sendall((uint8_t*)&c,sizeof(c));
                            skbase* pcam = new skcam(s,c);
                            pcam->name(mac);
                            _q.push(pcam);
                        }
                        else
                        {
                            s.sendall((uint8_t*)&c,sizeof(c));
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool    sksrv::_on_cli()
{
    skbase s;

    if(_cli.accept(s)>0)
    {
        urlreq             req;
        HttpRequestParser   parser;
        char                enough[1024];
        int                 bytes = s.select_receive((uint8_t*)enough,
                                                     sizeof(enough)-1,10000,2);
        if(bytes<=0)
        {
            s.destroy();
            return false;
        }
        enough[bytes]=0;
        LI("CONNECCTION ");
        LD(">:" << enough );
        const char* end = ::strstr(enough,"\r\n\r\n");
        if(end==nullptr)end=(enough + bytes);
        parser.parse(req, enough,end);

        if(req.method=="GET") //is a web
        {
            return _on_player(s,req);
        }
    }
    LI("CONNECCTION GONE");
    s.destroy();
    return false;
}

bool    sksrv::_display(skbase& s, const std::string& r)
{
    return true;
}

bool    sksrv::_on_player(skbase& s, const urlreq& req)
{
    std::vector<std::string> urlargs;
    ::split(req.uri,'?',urlargs);
    LI( req.uri );
    if(urlargs.size())
    {
        std::string mac = urlargs[0].substr(1);
        std::map<std::string, ClisWait>::iterator cli = _cliswait.find(mac);
        if(cli != _cliswait.end()) // no camera was here yet
        {
            _authcli(s, cli->second, mac, urlargs);
            if(cli->second.aok==true)
            {
                const skcam*  pcs = _p.has(mac);
                if(pcs && !_p.has(s))
                {
                    LI("new pol ");
                    skimg* pweb = new skimg(s);
                    pweb->name(mac);
                    _q.push(pweb);
                    return true;
                }
            }
        }
    }
    s.destroy();
    LI("CON GONE ");
    return false;
}

/**
 * @brief _authcli
 * @param cw
 * @param mac
 */
void sksrv::_authcli(skbase& s,
                     ClisWait& cw,
                     const std::string& mac,
                     const std::vector<std::string>& args)
{
    // send challange token and wait the md5
    if(args.size()>1)
    {
        std::vector<std::string> params;
        ::split(args[1],'&',params);
        for(const auto& p : params)
        {
            size_t ce = p.find('=');
            if(ce!=std::string::npos)
            {
                std::string k = p.substr(0,ce);
                std::string v = p.substr(ce+1);

                if(k=="auth")
                {
                    if(cw.passw == v)
                    {
                        ::msleep(128);
                        cw.aok = true;
                        cw.config.client = 1;
                        cw.tstamp = time(0);
                        return;
                    }
                }
            }
        }

        char md5[40];

        if(args[1]=="token")
        {
            srand(time(0));
            std::string token = std::to_string(rand());//  + cw.config.authplay;
            std::string encri = cw.config.authplay;
            std::string dec = xdecrypt(encri, Campas);
            // dec is cam token
            char out[1024];

            std::string tt = token+dec;\
            tt+="-";
            tt+=Srvpas;
            std::cout << "tokdec = " << tt << "\n";
            mg_md5(md5, tt.c_str(), 0);
            cw.passw = md5; //we expect this
            std::cout << "psw = " << cw.passw << "\n";

            sprintf(out,HDR, token.length(), token.c_str());
            s.send(out, strlen(out));
            ::msleep(256);
        }
        else if(args[1]=="config")
        {
            ;
        }
    }
    s.destroy();
    throw s.type();
}

/**
 * @brief sksrv::_regcamera
 * @param mac
 * @param c
 */
bool    sksrv::_regcamera(const std::string& mac, config& cf)
{
    const auto& cam = _cliswait.find(mac);

    if(cam==_cliswait.end())
    {
        ClisWait        cw;

        cw.tstamp      = time(0);
        cw.config      = cf;
        cw.aok         = false;
        _cliswait[mac] = cw;
        return false;
    }
    cf = cam->second.config;
    return true;
}

void    sksrv::keepcam(const std::string& mac)
{
    AutoLock a(&_m);
    std::map<std::string, ClisWait>::iterator f = _cliswait.find(mac);
    if(f!=_cliswait.end())
    {
        f->second.tstamp=time(0);
    }
}
