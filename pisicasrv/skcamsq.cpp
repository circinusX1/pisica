#include "skcamsq.h"

skcamsq::skcamsq()
{
}

skcamsq::~skcamsq()
{
    AutoLock a(&_m);
    while(_q.size()) {
        delete _q.front();
        _q.pop_front();
    }
}

bool skcamsq::push(skbase* ps)
{
    AutoLock a(&_m);
    _q.push_back(ps);
    return true;
}

skbase* skcamsq::pop()
{
    AutoLock a(&_m);
    if(_q.size()) {
        skbase *ppc = _q.front();
        _q.pop_front();
        return ppc;
    }
    return nullptr;
}
