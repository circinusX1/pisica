
#include <string.h>
#include <assert.h>
#include "vf.h"
#include "skcamsq.h"
#include "main.h"

#define OUT_STREAM  std::cout

umutex       vf::_Mut;
vf           vf::_Frames[MAX_FRAMES];
int          vf::_Free=MAX_FRAMES;
static int SeqF = 0;

vf::vf(vf::ALLOCF b):_stack(false)
{
    _seq = ++SeqF;
    //std::cout << "\n NEW FRAME  "<< _seq << "\n";
    if(b==POOL)return; /**/
    _buff = new uint8_t[MAX_BUFF+1];
    _cap = MAX_BUFF;
    _stack = true;
}

vf::~vf()
{
    if(_stack)
        delete[] _buff;
}

size_t  vf::set(size_t bytes)
{
    size_t room = _cap - bytes;
    _accum += bytes;
    return 0;   // all in
}

void    vf::set_len(uint32_t fl)
{
    if(_cap < fl)
    {
        delete _buff;
        _buff = new uint8_t [fl+sizeof(size_t)];
        _cap=fl+sizeof(size_t);
    }
    _frmlen=fl;
}

void vf::print(int c)const
{
    LI (_seq << " Frm LEN:" <<
                 _frmlen << " Accum " <<
                 _accum << " bytes in "
                 << c  );
}

void vf::reset()
{
    _accum=0;
    _busy=false;
    _frmlen=0;
    _seq = ++SeqF;
}

void vf::save(int seq)
{
    char fn[64];
    sprintf(fn,"test%d.jpg",seq);
    FILE* pf = ::fopen(fn,"wb");
    if(pf)
    {
        ::fwrite(_buff,1, _accum, pf);
        ::fclose(pf);
    }
}

void     vf::recache()
{
    AutoLock guard(&_Mut);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        _Frames[i].reset();
    }
    vf::_Free=MAX_FRAMES;
}

void* vf::operator new(size_t osz)throw()
{
    AutoLock guard(&_Mut);
    static bool Exhaust = false;
    UNUS(osz);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        if(_Frames[i].busy()==false)
        {
            _Frames[i]._now = time(0);
            _Frames[i].reset();
            _Frames[i].busy(true);
            _Frames[i]._stack=false;
            --vf::_Free;
            Exhaust=false;
            return (void*)&_Frames[i];
        }
    }
    if(Exhaust==false)
    {
        Exhaust=true;
        OUT_STREAM << "dropping vf. Pusher failed to eat\n";
    }
    return nullptr; //warn supress
}

void   vf::operator delete(void* pv, std::size_t sz)
{
    AutoLock guard(&_Mut);
    UNUS(sz);
    vf* pvf = reinterpret_cast<vf*>(pv);
    pvf->reset();
    ++vf::_Free;
}

size_t vf::cache_room()
{
    AutoLock guard(&_Mut);
    return vf::_Free;
}

