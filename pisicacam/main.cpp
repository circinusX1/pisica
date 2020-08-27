
#include "main.h"
#include "frameclient.h"
#include "singleton.h"
#include "devvideo.h"
#include "config.h"
#include "ffmt.h"
#include "jpeger.h"
#include "vigenere.h"

bool                __alive = true;
umutex*             _pm;
static bool         _dconf = false;
std::string         _srvpsw;
std::string         _campsw;
std::string         _mac;

config       _cfg = {"http://localhost:8888",
                     "/dev/video0",
                     "", //acl
                     "", //acl1
                     "",
                     "",
                     "",
                     false,
                     0,
                     640,
                     480,
                     15,
                     100,
                     500,
                     128,
                     100,
                     1,
                     80,
                     1,
                     250,
                     100,
                     0,
                     8, // pix/nr * nr to eliminate noise pixels
                     1,
                     0,
                     1,
                     1
                    };

void setts(config* pc);


void ControlC (int i)
{
    (void)(i);
    __alive = false;
    printf("Exiting...\n");
}


void ControlP (int i)
{
    (void)(i);
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
    // srv srcpass campass
    if(argc!=5)
    {
        //              0               1   2               3           4
        std::cout << argv[0] << "  server server-password cam-password dev/video# \n";
        std::cout << "\n";
        exit(1);
    }

    outfilefmt*     ffmt = new jpeger(_cfg.quality);
    urlinfo u(argv[1]);
    streamq q;

    ::strcpy(_cfg.device, /*dev*/ argv[4]);
    devvideo* pdv = new devvideo(&_cfg);
    if(pdv->open())
    {
        config          cfg;
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
        time_t          now = ticksave;
        frameclient     c(setts, &q, (const urlinfo*)&u, argv[2]);

        ::srand (time(NULL));
        memcpy(&cfg,&_cfg,sizeof(cfg));
        cfg.dirty = false;
        _srvpsw = argv[2];
        _campsw = argv[3];
        _mac = ::macaddr();
        std::cout << "cam token:" << _mac << "\n";
        // mac


        _pm = &m;
        c.start_thread();
        std::cout << "MAC: " << _mac << "\n";
        while(__alive)
        {
            now = gtc();
            LOCK_(_pm,
            if(_cfg.dirty)
            {
                _cfg.dirty=false;
                memcpy(&cfg,&_cfg,sizeof(_cfg));
                pdv->close();
                delete pdv;
                pdv = new devvideo(&_cfg);
                if(pdv->open()==false)
                {
                    std::cout << "CANNOT REPLLY CONFIG\n";
                    __alive = false;
                }
            });

            sz = 0;
            err = 0;
            const uint8_t*  pb422 = pdv->read(w, h, sz, err);
            if(pb422)
            {
                //std::cout << "got:" << w << "x" << h << ":" << sz <<  "\n";
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
                            //std::cout << "enquing frame " << jpgsz << "\n";
                            q.enque(pf);
                        }
                        else
                        {
                            std::cout << "buffer to small \n";
                            exit(2);
                        }
                    }
                }
            }else
            {
                std::cout << "getting 422 failed\n";
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


void setts(config* pc)
{
    AutoLock a(_pm);
    ::memcpy(&_cfg,pc,sizeof(_cfg));
    ::strcpy(_cfg.acl,xencrypt(_mac,_campsw).c_str());
    _dconf=true;
}
