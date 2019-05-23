
#include <string.h>
#include <assert.h>
#include "frame.h"
#include "streamq.h"
#include "main.h"

umutex      frame::_Mut;
frame       frame::_Frames[MAX_FRAMES];
int         frame::_Free=MAX_FRAMES;

bool  frame::add(const uint8_t* buff,
                                size_t bytes,
                                uint32_t frmlen)
{
    if((int)bytes <= room())
    {
        ::memcpy(_buff+_accum, buff, bytes);
        _accum+=bytes;
        return true;
    }
    return false;
}

void frame::tear()
{
    _accum=0;
    _busy=false;
    _frmlen=0;
}

void frame::save(int seq)
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

void     frame::zero()
{
    AutoLock guard(&_Mut);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        _Frames[i].tear();
    }
    frame::_Free=MAX_FRAMES;
}

void* frame::operator new(size_t osz)throw()
{
    AutoLock guard(&_Mut);
    static bool Exhaust = false;
    UNUS(osz);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        if(_Frames[i].busy()==false)
        {
            _Frames[i].tear();
            _Frames[i].busy(true);

            --frame::_Free;
            Exhaust=false;
            return (void*)&_Frames[i];
        }
    }
    if(Exhaust==false)
    {
        Exhaust=true;
        std::cout << "." << std::flush;
    }
    return nullptr; //warn supress
}

void   frame::operator delete(void* pv, std::size_t sz)
{
    AutoLock guard(&_Mut);
    UNUS(sz);
    frame* pvf = reinterpret_cast<frame*>(pv);
    pvf->tear();

    ++frame::_Free;
}

size_t frame::carom()
{
    AutoLock guard(&_Mut);
    return frame::_Free;
}

