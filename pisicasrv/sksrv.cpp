﻿#include <unistd.h>
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

#ifdef DEBUG
#   define TTL_CLI 120
#else
#   define TTL_CLI 16
#endif

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
    COUT_("SKSRV");
    p.link(this);
}

sksrv::~sksrv()
{
    COUT_("~SKSRV");
    _cli.destroy();
    _cam.destroy();
}

//srvpass, cliport, CamPort, loop_callback
bool sksrv::spin(const char* srvpass, int cliport, int camport , pfn_cb cb)
{
    if(_cam.create(camport, SO_REUSEADDR, 0)>0)
    {
        _srvpass = srvpass;
        fcntl(_cam.socket(), F_SETFD, FD_CLOEXEC);
        _cam.set_blocking(0);
        if(_cam.listen(4)!=0)
        {
            _cam.destroy();
            return false;
        }
        LI("CAM listening port "<< camport);
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
        LI("WEB listening port "<< cliport);
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
    skbase s(skbase::CAM);
    bool   rv=false;

    if(_cam.accept(s)>0)
    {
        struct   config  config;

        std::cout << "NEW CAM \n";
        char     local[1024]={0};
        int      len = s.receive((uint8_t*)local, sizeof(local));
        COUT_("CAM RECEIVED " << len);

        memcpy(&config, local, sizeof(config));

        if(len==sizeof(config) && config.sig==0xA5C1C0F0)
        {
            std::cout << "NEW CAM GOT " << len << "\n";
            std::string campas = xdecrypt(std::string(config.acl), Srvpas);
            std::string mac    = xdecrypt(std::string(config.acl1), campas);
            std::string rand   = xdecrypt(std::string(config.acl2), campas);

            if(mac == config.mac && ::atoi(rand.c_str()) == config.rndm)
            {
                _regcamera(mac, config, campas);
                config.timelapse = 0;

                if(s.sendall((uint8_t*)&config,sizeof(config)))
                {
                    // next statement detaches the socket from s
                    skbase* pcam = new skcam(s,config,this);
                    pcam->name(mac);
                    _q.push(pcam);
                }
                rv = true;
            }
            else
            {
                std::cout << "NEW CAM AUTH FAILED \n";
                s.destroy();
                rv=false;
            }
        }
        else{
            std::cout << "NEW CAM INVALID LEN \n";
            s.destroy();
            rv=false;
        }
    }
    return rv;
}

bool    sksrv::_on_cli()
{
    skbase s(skbase::CLIENT);

    if(_cli.accept(s)>0)
    {
        urlreq             req;
        HttpRequestParser   parser;
        char                enough[1024];
        int                 bytes = s.select_receive((uint8_t*)enough,
                                                     sizeof(enough)-1,10000,2);
        if(bytes<=0)
        {
            COUT_("CLI INVALID RECEIVE " << bytes << " BYTES");
            s.destroy();
            return false;
        }
        enough[bytes]=0;

        if(strstr(enough,"favico"))
        {
            COUT_("CLI ASKING FOR ICON " << bytes << " BYTES");
            s.destroy();
            return false;
        }

        LI("NEW CONNECTION ");
        LD(">:" << enough );
        const char* end = ::strstr(enough,"\r\n\r\n");
        if(end==nullptr)end=(enough + bytes);
        parser.parse(req, enough,end);

        if(req.method=="GET") //is a web
        {
            for(const auto& it: req.headers)
            {
                if(it.name=="Host")
                {
                    _srvurl=it.value;
                    break;
                }
            }
            return _on_player(s,req);
        }
    }
    LI("CONNECTION GONE");
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
        if(urlargs.size()==1)
        {
            char hdr[sizeof(HDR)+120];
            if(urlargs[0]=="/")
            {
                std::string reply = "<li>CAMERAS<ul>";
                for (const auto& a: _cliswait)
                {
                    reply += "<li>cameras: <a href='http://";
                    reply += _srvurl + "/";
                    reply += a.first + "'>";
                    reply += a.first +"</a></li>";
                }
                reply += "</ul>";
                int len = ::sprintf(hdr,HDR,reply.c_str(),"");
                s.send(hdr, len);
                s.send(reply.c_str(), reply.length());
            }
            else{
                std::string mac = urlargs[0].substr(1);
                std::map<std::string, ClisWait>::iterator cli = _cliswait.find(mac);
                if(cli != _cliswait.end()) // no camera was here yet
                {
                    std::string mac = urlargs[0].substr(1);
                    std::string reply;

                    cli->second.config.dirty = PROPS_CHANGED;
                    cli->second.config.webclienton = WEB_CLION_FRAMES;

                    if(cli->second.passw.at(0)=='_') // show if cam pass starts with _
                    {
                        reply += "<a href='http://";
                        reply += _srvurl + "/";
                        reply += mac;
                        reply += "?server=";
                        reply += Srvpas;
                        reply += "&camera=";
                        reply += cli->second.passw;
                        reply += "&image=1'>";
                        reply += "DEMO LINK TO STREAM";
                        reply += cli->first;
                        reply += "</a>";

                        reply += "<li>width: ";reply += std::to_string(cli->second.config.w);
                        reply += "<li>height: ";reply += std::to_string(cli->second.config.h);
                        reply += "<li>fps: ";reply += std::to_string(cli->second.config.fps);
                        reply += "<li>motion: ";reply += std::to_string(cli->second.config.motion);
                        reply += "<li>dev: ";reply += cli->second.config.device;
                        reply += "<li>tlapse: ";reply += std::to_string(cli->second.config.timelapse);
                        reply += "<li>mdiff: ";reply += std::to_string(cli->second.config.motiondiff);
                        reply += "<li>ml: ";reply += std::to_string(cli->second.config.motionl);
                        reply += "<li>mw: ";reply += std::to_string(cli->second.config.motionw);
                        reply += "<li>mh: ";reply += std::to_string(cli->second.config.motionh);

                        int len = ::sprintf(hdr,HDR,reply.c_str(),"");
                        s.send(hdr, len);
                        s.send(reply.c_str(), reply.length());
                    }
                    else {
                        cli->second.config.dirty = PROPS_CHANGED;
                        cli->second.config.webclienton = WEB_CLION_FRAMES;

                        reply += "<li>Online: ";
                        reply += mac + "</li>";
                    }
                }
            }
        }
        else {
            std::string mac = urlargs[0].substr(1);
            std::map<std::string, ClisWait>::iterator cli = _cliswait.find(mac);

            if(cli != _cliswait.end()) // no camera was here yet
            {
                _authcli(s, cli->second, mac, urlargs);

                if(cli->second.aok==true &&
                        urlargs.size()>1 &&
                        (urlargs[1].find("image")!=std::string::npos &&
                         urlargs[1].find("server")!=std::string::npos &&
                         urlargs[1].find("camera")!=std::string::npos))
                {
                    const skcam*  pcs = _p.has(mac);
                    if(pcs && !_p.has(s))
                    {
                        LI("new pool");
                        skimg* pweb = new skimg(s);
                        pweb->name(mac);
                        _q.push(pweb);
                        return true;
                    }
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
    if(args.size()>1)
    {
        int                      passcckeck=3;
        std::vector<std::string> params;

        ::split(args[1],'&',params);
        for(const auto& p : params)
        {
            size_t ce = p.find('=');
            if(ce!=std::string::npos)
            {
                std::string k = p.substr(0,ce);
                std::string v = p.substr(ce+1);

                if(k=="server")
                {
                    passcckeck -= v==Srvpas ? 1 : 0;
                }
                else if(k=="camera")
                {
                    passcckeck -= v==cw.passw ? 1 : 0;
                }
                else if(k=="image")
                {
                    passcckeck--;
                }

                if(passcckeck == 0)
                {
                    cw.aok = true;
                    cw.config.webclienton = WEB_CLION_FRAMES;
                    cw.config.dirty = PROPS_CHANGED;
                    cw.tstamp = time(0);
                    return;
                }

                if(k=="config")
                {
                    if(cw.passw == v)
                    {
                        LI("configuring by params");
                        _config_cam(mac, args);
                        s.destroy();
                        return;
                    }
                }
            }
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
void  sksrv::_regcamera(const std::string& mac, config& cf, const std::string& cpassw)
{
    const auto& cam = _cliswait.find(mac);

    if(cam==_cliswait.end())
    {
        ClisWait        cw;


        std::cout << "NEW CAM OK REPLY CONFIG AUTH \n";
        cw.tstamp      = time(0);
        cw.config      = cf;  //  save what camera config has
        cw.aok         = false;
        cw.passw       = cpassw;
        _cliswait[mac] = cw;
    }
    else {
        int rand = cf.rndm;
        std::string acl2 = cf.acl2;
        cf = cam->second.config;//  apply back if a web client chnanged it
        cf.rndm = rand;
        ::strcpy(cf.acl2, acl2.c_str());
    }
    keepcam(mac);
}

void    sksrv::keepcam(const std::string& mac)
{
    AutoLock a(&_m);
    std::map<std::string, ClisWait>::iterator f = _cliswait.find(mac);
    if(f!=_cliswait.end())
    {
        f->second.tstamp=time(0);

        f->second.config.webclienton =  WEB_CLION_FRAMES;
        f->second.config.dirty = PROPS_CHANGED;
    }
}

void sksrv::_config_cam(const std::string& mac, const std::vector<std::string>& args)
{
    AutoLock a(&_m);
    std::map<std::string, ClisWait>::iterator f = _cliswait.find(mac);
    if(f!=_cliswait.end())
    {
        f->second.tstamp=time(0);
        f->second.config.dirty = PROPS_CHANGED;
    }
}
