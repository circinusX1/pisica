#ifndef LISTENERS_H
#define LISTENERS_H

#include "sock.h"
#include "../common/config.h"

#define MIN_HDR  20
typedef void (*pfn_cb)();

class skbase;
class skcamsq;
class sks;
struct urlreq;
struct config;

struct ClisWait{
    bool                aok;
    struct config       config;
    time_t              tstamp;
    std::string         passw;
};

class sksrv
{
public:
    sksrv(sks& p, skcamsq& q);
    ~sksrv();
    bool    spin(const char* auth, int cport, int cliport, pfn_cb cb);
    void    keepcam(const std::string& mac);
private:
    int     _fd_set(fd_set&);
    void    _fd_check(fd_set&, int);
    bool    _on_cam();
    bool    _on_cli();
    bool    _display(skbase&, const std::string&);
    bool    _on_play(skbase&,   const urlreq&, const std::string&);
    bool    _on_player(skbase&, const urlreq& req);
    void    _regcamera(const std::string& mac, config& c, const std::string& s);
    void    _authcli(skbase& s,
                     ClisWait& cw,
                     const std::string& mac,
                     const std::vector<std::string>& args);
    void _config_cam(const std::string& mac, const std::vector<std::string>& args);

private:
    umutex          _m;
    sks&            _p;
    skcamsq&        _q;
    tcp_srv_sock    _cam;
    tcp_srv_sock    _cli;
    std::string     _srvpass;
    std::string     _srvurl;
    std::map<std::string, ClisWait> _cliswait;
};

#endif // LISTENERS_H
