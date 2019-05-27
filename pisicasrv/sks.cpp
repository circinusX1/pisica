
#include "main.h"
#include "sks.h"
#include "skcam.h"
#include "skweb.h"
#include "sksrv.h"
#include "skcamsq.h"

sks::~sks()
{
    closeall();
    signal_to_stop();
}

void     sks::closeall()
{
    //_tm.update(0,0);
    for(auto& a : _pool)
    {
        DELETE_PTR(a.second->_cam);
        camclis* pr = a.second;
        if(pr->_clis.size())
        {
            for(auto& csc: pr->_clis)
            {
                DELETE_PTR(csc);
            }
            pr->_clis.clear();
        }
        delete pr;
    }
    _pool.clear();
}

skcam*   sks::has(const std::string& name)
{
    AutoLock a(&_m);
    kconiterator it = _pool.find(name);
    return it==_pool.end() ?
                nullptr :
                (*it).second->_cam;
}

bool    sks::has(const skbase& sin)const
{
    for(const auto& a : _pool)
    {
        camclis* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            if(css->Rsin().c_str() == sin.Rsin().c_str())
            {
                return true;
            }
        }
    }
    return false;
}

sks::camclis*  sks::_has(const std::string& name)
{
    coniterator it = _pool.find(name);
    return it==_pool.end() ? nullptr : (*it).second;
}

std::string sks::show_cams(bool webreq)const
{
    AutoLock a(&_m);

    std::string ret;
    if(webreq)
        ret+="<pre>";
    for(const auto& a : _pool)
    {
        const skcam* pc = a.second->_cam;
        std::string  headers;
        ret+="&#60;img src='http://";
        ret+=sock::GetLocalIP("127");
        ret+=":";
        ret+=std::to_string(__cam_port);
        ret+="/";
        ret+=pc->name();
        ret+="'&#62;";
        ret+="\r\n";
    }
    ret+="\r\n";
    if(webreq)
        ret+="</pre>\n";
    // std::cout << ret << std::flush;
    return ret;
}

void sks::thread_main()
{
    fd_set  fdr;
    int     ndfs,dirty;

    while(!is_stopped() && __alive)
    {
        _new_conn(dirty);
        ndfs = _fd_set(fdr);
        dirty = _fd_check(fdr, ndfs);
    }
    closeall();
}

void sks::signal_to_stop()
{
    for(const auto& a : _pool)
    {
        if(a.second->_cam!=nullptr)
        {
            a.second->_cam->destroy();
        }
        const camclis* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            css->destroy();
        }
    }
    OsThread::signal_to_stop();
}

int sks::_fd_set(fd_set& fdr)
{
    int ndfs = 0;
    FD_ZERO(&fdr);
    for(const auto& a : _pool)
    {
        FD_SET(a.second->_cam->socket(), &fdr);
        ndfs = std::max(ndfs,a.second->_cam->socket());
        const camclis* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            if(css && css->isopen())
            {
                FD_SET(css->socket(), &fdr);
                ndfs = std::max(ndfs,css->socket());
            }
        }
    }
    return ndfs+1;
}

bool sks::_fd_check(fd_set& fdr, int ndfs)
{
    bool    dirty = false;
    timeval tv {0,200};
    int     sent;
    int     sel =::select(ndfs,&fdr,0,0,&tv);

    ++_lops;
    if(sel==-1){
        std::cerr << " socket select error critical \n";
        __alive=false;
        return false;    //dirty
    }
    if(sel==0){ return true; }
    for(auto& a : _pool)
    {
        if(!a.second->_cam->isopen()) {continue;}
        if(FD_ISSET(a.second->_cam->socket(),&fdr))
        {
            try{
                sent = a.second->_cam->ioio(a.second->_clis);
                if(sent && ((++_lops&0x1F)==0x1F)) //sent to clis
                {
                    _srv->keepcam(a.second->_cam->name());
                }
            }
            catch(skbase::STYPE& s)
            {
                dirty=_kill_kons(a.second, a.second->_cam, s);
            }
        }
        // we process locally anything from cclients
        const camclis* pr = a.second;
        if(pr->_clis.size())
        {
            std::vector<skbase*> cams;
            cams.push_back(pr->_cam);

            for(const auto& css: pr->_clis)
            {
                if(css && FD_ISSET(css->socket(),&fdr))
                {
                    try{
                        css->ioio(cams);
                    }
                    catch(skbase::STYPE& s)
                    {
                        dirty=_kill_kons(a.second, css, s);
                    }
                }
            }
        }
    }
    return dirty;
}

bool sks::_kill_kons(camclis* p, skbase* psock, const skbase::STYPE& s)
{
    int dirty=false;
    if(s==skbase::CAM)
    {
        ((skcam*)psock)->bind(nullptr,false);
        psock->destroy();
        if(p->_clis.size())
        {
            for(auto& cs : p->_clis)
            {
                cs->destroy();
            }
        }
        dirty=true;
    }
    else
    {
        ((skcam*)p->_cam)->bind((skweb*)psock,false);
        psock->destroy();
        dirty=true;
    }
    return dirty;
}

void sks::_new_conn(bool dirty)
{
    AutoLock a(&_m);

    skbase*  pcs = _q.pop();
    if(pcs)
    {
        sks::camclis* pexistentCam = _has(pcs->name());     /* a camera conn */

        if(pexistentCam==nullptr)
        {
            if(pcs->type()==skbase::CAM) // is camera socket
            {
                _kam_in(pcs);
                return;
            }
        }
        else
        {
            if(pcs->type()==skbase::CLIENT) // check if is a Client socket
            {
                _kli_in(pexistentCam, pcs);
                return;
            }
        }
        /// we dont know the request
        delete pcs;
    }
    if(dirty)
    {
        _clean();
        //_tm.update(_pool.size(),0);
    }
}

void sks::_kill_sck(const urlreq& req)
{
    AutoLock a(&_m);
}

void sks::_clean()
{
    AutoLock a(&_m);
AGAIN:
    coniterator it = _pool.begin();
    for(;it!=_pool.end();++it)
    {
        camclis* pr =   (*it).second;
        //
        // if camera got away
        //
        if(!_expired.empty())
        {
            if(_expired==pr->_cam->name())
            {
                config cfg = pr->_cam->configget();
                cfg.client=0;
                pr->_cam->configit(cfg);
            }
            _expired.clear();
        }

        if(pr->_cam->isopen()==false)
        {
            for(auto& client : pr->_clis)
            {
                _kli_out(pr, client);
                client->destroy();
                DELETE_PTR(client);
            }
            pr->_clis.clear();
            _kam_out(pr->_cam);
            DELETE_PTR (pr->_cam);
            _pool.erase(it);
            DELETE_PTR(pr);
            LI( " STREAM GONE. DROPPING CLIENTS TOO");
            goto AGAIN;
        }
        //
        // if a client got away
        //
        if(pr->_clis.size())
        {
DELTAG:
            std::vector<skbase*>::iterator it = pr->_clis.begin();
            for(;it!=pr->_clis.end();it++)
            {
                if((*it)->isopen()==false)
                {
                    if(pr->_cam)
                    {
                        pr->_cam->bind((skweb*)*it,false);
                    }
                    _kli_out(pr, *it);
                    (*it)->destroy();
                    DELETE_PTR(*it);
                    pr->_clis.erase(it);
                    goto DELTAG;
                }
            }
        }
    }
}

void sks::_kli_out(camclis* pr, skbase* cs)
{
    LI("Client disconnected");
    if(pr->_cam && pr->_cam->isopen())
    {
        pr->_cam->sendall("client_off",7);
    }
}

void sks::_kli_in(camclis* pcamerain, skbase* pcs)
{
    pcamerain->_clis.push_back(pcs);
    pcamerain->_cam->bind((skweb*)pcs,true);
    //pcamerain->_cam->sendall("client_on",6);
}

bool sks::has_cam(const std::string& pcsname)
{
    AutoLock a(&_m);
    return _pool.find(pcsname)!=_pool.end();
}

void sks::_kam_in(skbase* pcs)
{
    camclis* p                 = new camclis();

    p->_cam = (skcam*)(pcs);
    p->_cam->bind(nullptr,false);
    p->_clis.clear();
    _pool[pcs->name()] = p;
    std::string streamname = pcs->name();
}

void sks::_kam_out(skcam* cs)
{

}

void sks::cliexpired(const std::string& mac)
{
    AutoLock a(&_m);
    _expired = mac;
}
