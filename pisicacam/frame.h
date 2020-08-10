#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include "osthread.h"
#include <iostream>
#include <stdint.h>
#include <vector>

#define MAX_BUFF   (1024*480)
#define MAX_FRAMES 8

class streamq;
class frame
{
public:
    explicit frame(bool)
    {
    }

    virtual ~frame(){};

    static size_t carom();
    void     tear();
    bool     busy()const {return _busy;};
    void     busy(bool b){_busy=b;_accum=0; _now=time(0);};
    size_t   now()const{return _now;}
    uint8_t* buffer(){return _buff;}
    void     bufend(size_t be){_accum=be;}
    bool     add(const uint8_t* buff, size_t bytes, uint32_t required=0);
    size_t   length()const{return _accum;}
    size_t   size()const{return MAX_BUFF;}
    bool     hasRoom(size_t r){return MAX_BUFF-(_accum)>r;}
    void     save(int);
    int      room()const{return (int)(MAX_BUFF-(_accum));}
    uint32_t frmlen()const{return _frmlen;}
private:
    explicit frame():_accum(0),_cap(MAX_BUFF+1)
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
    static void     zero();

private:
    size_t   _accum;
    size_t   _cap;
    bool     _busy;
    size_t   _frames;
    uint32_t _frmlen;
    size_t   _now;
    uint8_t* _buff;
private:

    static umutex       _Mut;
    static frame   _Frames[MAX_FRAMES];
    static uint8_t*     _vchunks;
    static size_t       _vaccum;
    static int          _Free;
};

#endif // VIDEOFRAME_H
