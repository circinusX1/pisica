#ifndef SK_IMG
#define SK_IMG

#include <stddef.h>
#include <stdint.h>
#include "skweb.h"

class skimg : public skweb
{
public:
    skimg(skbase&);
    virtual ~skimg();
    virtual int snd(const uint8_t* b, size_t room,uint32_t extra=0);

private:
    bool    _hok=false;
};

#endif // SK_IMG
