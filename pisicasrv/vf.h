#ifndef FRAME_H
#define FRAME_H

#include "osthread.h"
#include <iostream>
#include <stdint.h>
#include <vector>

#define MAX_BUFF   (1024*256)
#define MAX_FRAMES  128       //min 1 sec


class skcamsq;
class vf
{
public:
    typedef enum{STACK=0,POOL}ALLOCF;
    explicit vf(vf::ALLOCF b);
    virtual ~vf();
    static size_t cache_room();
    void     reset();
    int      seq()const{return _seq;}
    void     print(int)const;
    bool     busy()const {return _busy;};
    void     busy(bool b){_busy=b;_accum=0;};
    const uint8_t* buffer()const{return _buff;}
    void     bufend(size_t be){_accum=be;}
    size_t   set(size_t bytes);
    size_t   length()const{return _accum;}
    size_t   size()const{return MAX_BUFF;}
    void     save(int);
    int      room()const{return (int)(MAX_BUFF-(_accum));}
    uint32_t   frmlen()const{return _frmlen;}
    void    set_len(uint32_t fl);

private:
    explicit vf():_accum(0),_cap(MAX_BUFF)
    {
        _buff = new uint8_t[MAX_BUFF+1];
        if(_buff==0)
        {
            printf("cannot allocate heap \r\n");
            exit(0);
        }
    }
    bool _resync(size_t& offset);

public:
    static void*    operator new(size_t sz)throw();
    static void     operator delete(void* pv, std::size_t sz);
    static void     recache();
public:
    size_t      _now;
protected:
    size_t      _accum;
    size_t      _cap;
    bool        _busy;
    size_t      _nframes;
    uint32_t    _frmlen;
    uint8_t*    _buff;
    int         _seq;

    bool        _stack=false;
private:

    static umutex       _Mut;
    static vf           _Frames[MAX_FRAMES];
    static uint8_t*     _vchunks;
    static size_t       _vaccum;
    static int          _Free;
};

#define FROM_POOL true

#endif // VIDEOFRAME_H
