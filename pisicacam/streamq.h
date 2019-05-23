#ifndef STREAMQ_H
#define STREAMQ_H

#include "osthread.h"
#include <deque>
#include "frame.h"


typedef enum{
    eFULL = 1,
    eNARROW = 2,
}E_CACHE;


class streamq
{
public:
    streamq(){};
    ~streamq();

    frame* deque();
    void enque(frame*);
private:
    std::deque<frame*>     _frames;
    umutex                 _m;
};

#endif // FRAMEQUEUE_H
