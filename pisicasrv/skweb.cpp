#include "skweb.h"
#include "vf.h"
#include "skcamsq.h"

skweb::skweb(skbase& o,
                 STYPE st):skbase(o,st)
{
    COUT_("SKWEB");
}

skweb::~skweb()
{
    COUT_("~SKWEB");
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
    COUT_("WEB DESTORY");
    return skbase::destroy(be);
}
