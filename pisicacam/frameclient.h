#ifndef PUSHER_H
#define PUSHER_H

#include "main.h"
#include "osthread.h"
#include "sock.h"
#include "streamq.h"
#include "urlinfo.h"
#include "config.h"
#include <iostream>
//#include <curl/curl.h>

#define MAX_NO_FRAMES   500
typedef void (*cbnoty)(config* pc);

class frameclient : public OsThread
{
public:
    frameclient(cbnoty ns, streamq* pq, const urlinfo* url, const char* auth);
    virtual ~frameclient();
    void    stop_thread();
    void    drop(bool running=true);
    void    keep_alive(){_noframe = MAX_NO_FRAMES;}
protected:
    virtual void thread_main();
    void _stream(tcp_cli_sock& cli);
    void _send();

private:
    streamq*        _pq;
    time_t          _now;
    size_t          _bps;
    cbnoty          _cbn;
    const urlinfo*  _url;
    std::string    _auth="marius";
    int            _connectfail=0;
    time_t         _lastfrmtime;
    int            _noframe=MAX_NO_FRAMES;
    config         _fconf;
    bool           _allowed=false;
};

#endif // PUSHER_H
