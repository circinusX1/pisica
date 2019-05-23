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
#include "md5.h"

static char HDR[] = "HTTP/1.0 200 OK\r\n"
                    "Connection: close\r\n"
                    "Server: pisica/1.0\r\n"
                    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                    "Pragma: no-cache\r\n"
                    "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
                    "Content-type: text/html\r\n"
                    "Content-length: %d\r\n"
                    "X-Timestamp: %d.%06d\r\n\r\n";

sklsn::sklsn(sks& p, skcamsq& q):_p(p),_q(q)
{
}

sklsn::~sklsn()
{
    _cli.destroy();
    _cam.destroy();
}

bool    sklsn::spin(const char* auth, int cport, int cliport, pfn_cb cb)
{
    if(_cam.create(cport, SO_REUSEADDR, 0)>0)
    {
        _auth = auth;
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
    while(__aware)
    {
        ::msleep(8);
        _fd_check(r, _fd_set(r));
        cb();
    }
    return true;
}

int   sklsn::_fd_set(fd_set& rd)
{
    int ndfs = max(_cam.socket(),_cli.socket())+1;
    FD_ZERO(&rd);
    FD_SET(_cam.socket(), &rd);
    FD_SET(_cli.socket(), &rd);
    return ndfs;
}

void sklsn::_fd_check(fd_set& rd, int ndfs)
{
    timeval tv {0,0xFFFF};
    int     is = ::select(ndfs, &rd, 0, 0, &tv);
    if(is ==-1) {
        std::cerr << "socket select() critical " << "\n";
        __aware=false;
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

bool    sklsn::_on_cam()
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
            const std::string tok = e; // ::dekr(e,_auth);
            size_t delim = tok.find("-");
            const std::string mac = tok.substr(0,delim);
            if(mac.length()==12)
            {
                std::string toenc = e + _auth;
                ::mg_md5(replymd5, toenc.c_str(), nullptr);
                if(s.sendall(replymd5,strlen(replymd5))==(int)strlen(replymd5))
                {
                    int l = s.receive((uint8_t*)enough,4);
                    if(l==4)
                    {
                        if(_p.has(mac))  // no camera with this mac in pool
                        {
                            LW("Conection refused: Already streaming from this MAC");
                            return false;
                        }
                        skbase* pcam = new skcam(s);
                        pcam->name(mac);
                        _q.push(pcam);
                    }
                }
            }
        }
    }
    return true;
}

bool    sklsn::_on_cli()
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

            if(req.uri.length()==13) // is a image link <img src='/?MAC'> We wnforce an image with a ?
            {
                return _on_jpeg(s,req, req.uri.substr(1));
            }
            return _display(s, std::string(enough)); //we dont know what it is
        }

    }
    return true;
}

bool    sklsn::_display(skbase& s, const std::string& r)
{
    if(r.find("HTTP") != std::string::npos) // is a web, send the header
    {
        char    hdr[512];
        struct  timeval timestamp;
        struct  timezone tz = {5,0};

        std::string channels = _p.show_cams(true);
        gettimeofday(&timestamp, &tz);
        sprintf(hdr,HDR,  channels.length(),
                (int) timestamp.tv_sec,
                (int) timestamp.tv_usec);
        s.snd(hdr,strlen(hdr));
        s.snd(channels.c_str(),channels.length());
        return true;
    }
    std::string channels = _p.show_cams(false); // send the text
    s.snd(channels.c_str(),channels.length());

    return true;
}

bool    sklsn::_on_jpeg(skbase& s,
                          const urlreq& htr, const std::string& r)
{
    const skcam* pcs = _p.has(r);
    if(pcs && !_p.has(s)) //one client for this stream
    {
        skimg* pcam = new skimg(s);
        pcam->name(r);
        _q.push(pcam);
        return true;
    }
    return _display(s,r);
}
