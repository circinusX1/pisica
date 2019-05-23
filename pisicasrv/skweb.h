#ifndef CLISOCK_H
#define CLISOCK_H

#include <stddef.h>
#include <stdint.h>
#include "skbase.h"
class skcamsq;
class skweb : public skbase
{
public:
    skweb(skbase&, STYPE t=skbase::CLIENT);
    virtual ~skweb();

    virtual int snd(const uint8_t* b, size_t room, uint32_t extra=0);
    virtual int snd(const char* b, size_t room, uint32_t extra=0);
    virtual bool destroy(bool be=true);

protected:

};

#endif // CLISOCK_H
