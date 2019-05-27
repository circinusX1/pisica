#ifndef SKS_H
#define SKS_H


#include <string>
#include <map>
#include "skbase.h"
#include "logfile.h"

struct urlreq;
class skcamsq;
class skweb;
class skcam;
class sksrv;
class sks : public OsThread
{
public:
    struct camclis{
        skcam* _cam;
        std::vector<skbase*> _clis;
    };
    sks(skcamsq& q):_q(q){
    }
    ~sks();
    skcam*    has(const std::string& name);
    bool        has(const skbase& sin)const;
    std::string show_cams(bool webreq)const;
    void closeall();
    void thread_main();
    void signal_to_stop();
    void _kill_sck(const urlreq& req);
    bool has_cam(const std::string& pcsname);
    void link(sksrv *srv){_srv=srv;}
    void cliexpired(const std::string& mac);
private:
    int _fd_set(fd_set& fdr);
    bool _fd_check(fd_set& fdr, int ndfs);
    void _new_conn(bool);
    void _clean();
    camclis*  _has(const std::string& name);
    bool _kill_kons(camclis* p, skbase* psock, const skbase::STYPE& s);
    void _kam_in(skbase* pcs);
    void _kli_in(camclis*,skbase* pcs);
    void _kli_out(camclis*,skbase* cs);
    void _kam_out(skcam* cs);

private:
    typedef std::map<std::string,camclis*>::iterator coniterator;
    typedef std::map<std::string,camclis*>::const_iterator kconiterator;
    skcamsq&                        _q;
    std::map<std::string,camclis*>  _pool;
    umutex                          _m;
    sksrv*                          _srv = nullptr;
    int                             _lops = 0;
    std::string                     _expired;
};

#endif // POOL_H
