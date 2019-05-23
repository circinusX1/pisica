#include "skweb.h"
#include "vf.h"
#include "skcamsq.h"

skweb::skweb(skbase& o,
                 STYPE st):skbase(o,st)
{

}

skweb::~skweb()
{
}

int skweb::snd(const uint8_t* b, size_t room, uint32_t extra)
{
    return skbase::snd(b,room,extra);
}

int skweb::snd(const char* b, size_t room, uint32_t extra)
{
    return skbase::snd(b,room,extra);
}

bool skweb::destroy(bool be)
{
    return skbase::destroy(be);
}
