
#include <assert.h>
#include "main.h"
#include "streamq.h"


#define nullout std::cout


streamq::~streamq()
{
    AutoLock guard(&_m);
    while(_frames.size())
    {
        frame* pv = _frames.front();
        _frames.pop_front();
        delete pv;
    }
}

void streamq::enque(frame* vf)
{
    AutoLock guard(&_m);
    if(vf->length())
        _frames.push_back(vf);
}

frame* streamq::deque()
{
    AutoLock guard(&_m);
    if(_frames.size())
    {
        frame* pv = _frames.front();
        _frames.pop_front();
        return pv;
    }
    return nullptr;
}
