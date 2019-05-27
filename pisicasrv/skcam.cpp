
#include <sys/stat.h>
#include <sys/types.h>
#include "main.h"
#include "skcam.h"
#include "skweb.h"
#include "../common/config.h"

skcam::skcam(skbase& o, const config& c):skbase(o,skbase::CAM),
    _vf(vf::STACK)
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
            _dconf=true;
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
        if(_pclis.size()==0)
        {
            AutoLock a(&_m);
            _dconf = true;
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
            if(bytes == (int)len)
            {
                do{
                    AutoLock a(&_m);
                    if(_dconf)
                    {
                        this->snd((const unsigned char*)&_c, sizeof(_c));
                        _dconf=false;
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

    if(_c.lapse || _c.motion)
    {
        _record();
    }
    _vf.reset();
    return ret;
}

void    skcam::configit(const config& c)
{
    AutoLock a(&_m);
    _c = c;
    _dconf = true;
}

void skcam::_record()
{
}

