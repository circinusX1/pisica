
#include <sys/stat.h>
#include <sys/types.h>
#include "main.h"
#include "skcam.h"
#include "skweb.h"
#include "sksrv.h"
#include "../common/config.h"

static char JpegHdr[]="Content-Type: image/jpeg\r\n"
                      "Content-Length: %d\r\n"
                      "X-Timestamp: %d.%d\r\n\r\n";

static char JpegPart[] = "\r\n--MariusMariusMarius\r\nContent-type: image/jpeg\r\n";
static char JpegPartHeader[] = "HTTP/1.0 200 OK\r\n"
                               "Connection: close\r\n"
                               "Server: pisica/1.0\r\n"
                               "Cache-Control: no-cache\r\n"
                               "Content-Type: multipart/x-mixed-replace;boundary=MariusMariusMarius\r\n"
                               "\r\n"
                               "--MariusMariusMarius\r\n";

skcam::skcam(skbase& o,
             const config& c,
             sksrv* psrv):skbase(o,skbase::CAM),_vf(vf::STACK),_psrv(psrv)
{

    bind(nullptr,false);
    _vf.reset();
    _now = time(0);
    _bps = 0;
    _c = c;
    LI("CAM ON "  << this->Rsin().c_str());
}

skcam::~skcam()
{
    LI("CAM DIS_CONNECTED " << this->Rsin().c_str());
}

bool skcam::destroy(bool be)
{
    return skbase::destroy(be);
}

void skcam::bind(skweb* pcs, bool addklie)
{
    if(addklie)
    {
        if(_pclis.find(pcs)==_pclis.end())
        {
            AutoLock a(&_m);
            _pclis.insert(pcs);
        }
    }
    else
    {
        if(pcs==nullptr)
        {
            _pclis.clear();
        }
        else
        {
            if(_pclis.find(pcs)!=_pclis.end())
            {
                _pclis.erase(pcs);
            }
        }
    }
}

int skcam::ioio(const std::vector<skbase*>& clis)
{
    uint32_t len = 0;
    uint32_t bytes = this->receiveall((unsigned char*)&len, sizeof(uint32_t));
    if(bytes==sizeof(uint32_t))
    {
        if(len && len <= (uint32_t)_vf.room())
        {
            _vf.reset();
            bytes = this->receiveall((unsigned char*)_vf.buffer(), len);
            if(bytes == (uint32_t)len)
            {
                do{
                    AutoLock a(&_m);
                    if(_c.dirty)
                    {
                        this->snd((const unsigned char*)&_c, sizeof(_c));
                        _c.dirty=false;
                    }
                }while(0);

                _vf.set(len);
                return _shoot(_vf);
            }
            else //cannot get frame
            {
                LW("remote colosed connetion. cannot get frame \n");
            }
        }
        else
        {
            LW("no room");
        }
    }
    LW("remote colosed connetion. received 0 bytes \n");
    throw (skbase::CAM);
    return 0;
}

int skcam::_shoot(const vf& vf)
{
    time_t now = time(0);
    int ret = 0;

    _bps += vf.length();
    if(now > _now+5)
    {
        _now=now;
        LI("Got: " << _bps/5 << " bps");
        _bps=0;
        _psrv->keepcam(_name);
    }
    if(_pclis.size())
    {
        for(const auto& cs : _pclis)
        {
            if(cs->isopen())
            {
                ret += cs->snd(vf.buffer(),vf.length(),0);
            }
        }
    }

    if(_c.lapse || _c.motion || _c.streame || _c.client)
    {
        _record(vf.buffer(),vf.length());
    }
    _vf.reset();
    return ret;
}

void    skcam::configit(const config& c)
{
    AutoLock a(&_m);
    _c = c;
    _c.dirty = true;
}

void skcam::_record(const uint8_t* pb, size_t l)
{
    return;
    std::string fn = "/tmp/"; fn+=name(); fn+=".jpg";
    bool nf = ::access(fn.c_str(),0)!=0;

    FILE* pf = ::fopen(fn.c_str(),"ab");
    if(pf)
    {
        char buffer[1024];
        struct timeval timestamp;
        struct timezone tz = {5,0};
        gettimeofday(&timestamp, &tz);

        if(nf){
            std::string htf = "/tmp/"; htf+=name(); htf+=".html";
            FILE* pf2 = ::fopen(htf.c_str(),"wb");
            ::fprintf(pf2,"<img src='%s'>",fn.c_str());
            ::fclose(pf2);
            ::fwrite((const uint8_t*)JpegPartHeader,1, strlen(JpegPartHeader), pf);
        }

        size_t hl = ::sprintf(buffer, JpegHdr,
                              (int)l,
                              (int)timestamp.tv_sec,
                              (int)timestamp.tv_usec);

        ::fwrite((const uint8_t*)buffer,1,hl,pf);
        ::fwrite(pb, 1, l, pf);
        ::fwrite((const uint8_t*)JpegPart,1,strlen(JpegPart),pf);
        ::fclose(pf);
    }
}

