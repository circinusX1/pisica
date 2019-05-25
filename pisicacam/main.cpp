
#include "main.h"
#include "frameclient.h"
#include "singleton.h"
#include "devvideo.h"
#include "../common/config.h"
#include "ffmt.h"
#include "jpeger.h"

bool                __alive = true;
static umutex*      _pm;
static bool         _dconf = false;
std::string         _mac;
config       _cfg = {0,
                         "http://localhost:8888",
                         "/dev/video0",
                         1366,
                         768,
                         15,
                         1,
                         8000,
                         1,
                         1,
                         1000,
                         80,
                         1,
                         10,
                         100,
                         0,
                         4,
                         1,
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         0,
                        0,
                        0

             };

void settigns(config* pc);


void ControlC (int i)
{
    __alive = false;
    printf("Exiting...\n");
}


void ControlP (int i)
{
}


int main(int argc, char *argv[])
{
    SingleProc p (4590);
    umutex     m;
    config     cfg;

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
    outfilefmt*     ffmt = new jpeger(_cfg.quality);
    urlinfo u("http://localhost:8888");
    streamq q;
    devvideo* pdv = new devvideo(&_cfg);
    if(pdv->open())
    {
        int             moinertia=0;
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

        _mac = ::macaddr();
        _pm = &m;
        c.start_thread();

        while(__alive)
        {
            now = gtc();
            do{
                AutoLock a(_pm);
                if(_dconf)
                {
                    cfg = _cfg;
                    _dconf=false;
                }
            }while(0);
            if(cfg.dirty)
            {
                pdv->close();
                delete pdv;
                pdv = new devvideo(&cfg);
                if(pdv->open()==false)
                {
                    break;
                }
                cfg.dirty=0;
            }

            sz = 0;
            err = 0;
            const uint8_t*  pb422 = pdv->read(w, h, sz, err);
            if(pb422)
            {
                jpgsz=ffmt->convert420(pb422, w, h, sz, cfg.quality, &pjpg);
                //
                // motion
                //
                if(cfg.motionl)
                {
                    movepix = pdv->movement();
                    if(--moinertia>0 || ( movepix >= cfg.motionl && movepix <= cfg.motionh))
                    {
                        //std::cout << "move pix=" << movepix << "\n";
                        bstream = true;
                        cfg.motion = movepix;
                        if( movepix >= cfg.motionl && movepix <= cfg.motionh)
                        {
                            moinertia = 15;
                        }
                    }
                    else
                    {
                        bstream = false;
                        cfg.motion =0;
                    }
                    tickmove = now;
                }

                if(cfg.timelapse > 0)
                {
                    if((now - lapsetick) > (uint32_t)cfg.timelapse)
                    {
                        bstream = true;
                        lapsetick = now;
                        cfg.lapse = 1;
                    }
                    else {
                        cfg.lapse = 0;
                    }
                }

                if(bstream==false)
                {
                    bstream = true;//cfg.client; // true;
                }
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
                            exit(2);
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


void settigns(config* pc)
{
    AutoLock a(_pm);
   _cfg = *pc;
   _dconf=true;
}
