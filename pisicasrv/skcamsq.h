#ifndef SKQ_H
#define SKQ_H

#include <deque>
#include "skbase.h"

class sks;
class skbase;
class skcamsq
{
public:
    skcamsq();
    virtual ~skcamsq();
    bool push(skbase*);
    skbase* pop();

private:
    deque<skbase*>   _q;
    umutex          _m;
};

#endif // SKQ_H
