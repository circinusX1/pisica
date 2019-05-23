#ifndef LISTENERS_H
#define LISTENERS_H

#include "sock.h"

#define MIN_HDR  20
typedef void (*pfn_cb)();

class skbase;
class skcamsq;
class sks;
struct urlreq;
class sklsn
{
public:
    sklsn(sks& p, skcamsq& q);
    ~sklsn();
    bool    spin(const char* auth, int cport, int cliport, pfn_cb cb);

private:
    int     _fd_set(fd_set&);
    void    _fd_check(fd_set&, int);
    bool    _on_cam();
    bool    _on_cli();
    bool    _display(skbase&, const std::string&);
    bool    _on_play(skbase&,   const urlreq&, const std::string&);
    bool    _on_jpeg(skbase&, const urlreq& htr, const std::string&);

private:
    sks&        _p;
    skcamsq&        _q;
    tcp_srv_sock    _cam;
    tcp_srv_sock    _cli;
    std::string     _auth;
};

#endif // LISTENERS_H
