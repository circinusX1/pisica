
#include <sys/stat.h>
#include <sys/types.h>
#include "main.h"
#include "skcam.h"
#include "skweb.h"

skcam::skcam(skbase& o):skbase(o,skbase::CAM),
    _vf(vf::STACK)
{

    bind(nullptr,false);
    _vf.reset();
    _now = time(0);
    _bps = 0;
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

void skcam::bind(skweb* pcs, bool addremove)
{
    if(addremove)
    {
        if(_pclis.find(pcs)==_pclis.end())
        {
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
    if(this->receive((unsigned char*)&len, sizeof(uint32_t))==sizeof(uint32_t))
    {
        if(len && len <= (uint32_t)_vf.room())
        {
            _vf.reset();
            if(this->receive((unsigned char*)_vf.buffer(), len) == (int)len)
            {
                _vf.set(len);
                _shoot(_vf);
                return 0;
            }
        }
    }
    throw (skbase::CAM);
}

void skcam::_shoot(const vf& vf)
{
    time_t now = time(0);
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
                cs->snd(vf.buffer(),vf.length(),0);
        }
    }
    _vf.reset();
}
