#include "main.h"
#include "skbase.h"
#include "sks.h"

skbase::skbase(skbase& r,
                   skbase::STYPE t):
                                tcp_cli_sock(r),
                                _t(t)
{
    r.detach();
}

skbase::~skbase()
{
}

int skbase::snd(const char* b, size_t room, uint32_t extra)
{
    //std::cout << "<:";
    //std::cout << b << std::flush;
    return snd((const uint8_t*)b,room,extra);
}

int skbase::snd(const uint8_t* b, size_t room, uint32_t extra)
{
    int rb = this->sendall(b,room);
    if((size_t)rb!=room)throw _t;
    // std::cout << "<:" << b << "\r\n";
    return rb;
}

bool skbase::destroy(bool be)
{
    return tcp_cli_sock::destroy(be);
}

bool    skbase::isopen()
{
    return tcp_cli_sock::isopen();
}


