
#include "main.h"
#include "frameclient.h"
#include "singleton.h"
#include "devvideo.h"
#include "config.h"
#include "ffmt.h"
#include "jpeger.h"

bool   __alive = true;
static umutex*     _pm;
static config      _config = Cfg;
std::string         _mac;

void ControlC (int i)
{
    __alive = false;
    printf("Exiting...\n");
}


void ControlP (int i)
{
}

void settigns(config* pc)
{
    AutoLock a(_pm);
    _config = *pc;
}

int main(int argc, char *argv[])
{
    SingleProc p (4590);
    umutex     m;
    signal(SIGINT,  ControlC);
    signal(SIGABRT, ControlC);
    signal(SIGKILL, ControlC);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    if(!p())
    {
        std::cout << argv[0] << " running\n";
        return -1;
    }
    outfilefmt*     ffmt = new jpeger(_config.quality);
    urlinfo u("http://localhost:8888");
    streamq q;
    devvideo* pdv = new devvideo(&_config);
    if(pdv->open())
    {
        uint32_t        jpgsz = 0;
        uint8_t*        pjpg = 0;
        size_t          sz;
        int             w,h;
        bool            err;
        uint32_t        lapsetick =  gtc();
        uint32_t        tickmove =  gtc();
        uint32_t        ticksave =  gtc();
        int             movepix = 0;
        bool            bstream = false;
        time_t          now;
        int             delay = 10;
        frameclient c(settigns, &q, (const urlinfo*)&u, "marius");

        _mac=::macaddr();

        _pm = &m;
        c.start_thread();

        while(__alive)
        {
            now = gtc();
            do{
                AutoLock a(_pm);
                if(_config.dirty)
                {
                    pdv->close();
                    delete pdv;
                    pdv = new devvideo(&_config);
                    if(pdv->open()==false)
                    {
                        break;
                    }
                    _config.dirty=0;
                }
            }while(0);
            sz = 0;
            err = 0;
            const uint8_t*  pb422 = pdv->read(w, h, sz, err);
            if(pb422)
            {
                jpgsz=ffmt->convert420(pb422, w, h, sz, _config.quality, &pjpg);
                //
                // motion
                //
                if(_config.motionl)
                {
                    if(now-tickmove > _config.motionsnap)
                    {
                        movepix = pdv->movement();
                        if( movepix >= _config.motionl && movepix <= _config.motionh)
                        {
                            std::cout << "move pix=" << movepix << "\n";
                            bstream = true;
                        }
                        else
                        {
                            bstream = false;
                        }
                        tickmove = now;
                    }
                }

                if(_config.timelapse > 0)
                {
                    if((now - lapsetick) > (uint32_t)_config.timelapse)
                    {
                        bstream = true;
                        lapsetick = now;
                    }
                }

                if(pdv->darkaverage() < _config.darkaverage)
                {
                    bstream = false;
                }
                if(pdv->darkaverage() < _config.darkmotion)
                {
                    bstream = false;
                }
                bstream=true;
                if(bstream)
                {
                    frame* pf = new frame(true);
                    if(pf)
                    {
                        pf->add((uint8_t*)&jpgsz, sizeof(jpgsz), 0); // 4 bytes length of the frame
                        if(pf->add(pjpg, jpgsz, 0))
                        {
                            q.enque(pf);
                        }
                        else
                        {
                            std::cout << "buffer to small \n";
                        }
                    }
                }
            }
            bstream = false;
            ticksave = now;
            ::msleep(50);
        }
    }
    pdv->close();
    delete pdv;
    delete ffmt;
    return 0;
}
