
#include "main.h"
#include "frameclient.h"
#include "singleton.h"
#include "devvideo.h"
#include "config.h"
#include "ffmt.h"
#include "jpeger.h"
#include "vigenere.h"

#ifdef DEBUG
#   define MOTION_INTERTTIA    3
#else
#   define MOTION_INTERTTIA    30
#endif

bool                __alive = true;
umutex*             _pm;
std::string         _srvpsw;
std::string         _campsw;
std::string         _mac;

config       _cfg = {"http://localhost:8888",
                     "/dev/video0",
                     "acl", //acl
                     "acl1", //acl1
                     "acl2", //acl1
                     "mac", //acl2
                     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},    // motionrejectrects
                     false, // dirty
                     0xFF,     // random
                     640,   // w
                     480,   // h
                     30,    // fps
                     0,    // motion pix min / frame
                     0,    // (disabled) motion pix max per frame (camera pan protect all pix move)
                     64,    // motion width rect b&w pixels
                     20,    // motion window pixels rect width
                     80,    // jpg quality 80%
                     0,     // timelapse
                     250,   // dark image
                     100,   // darm moption pix
                     0,     // rotate
                     4,     // pix/nr * nr to eliminate noise pixels
                     0,     // is there a web webclienton watching
                     0,     // was motion r/w
                     "1234567",
                     0xA5C1C0F0,
                    };

void configcam(config* pc);


void ControlC (int i)
{
    (void)(i);
    __alive = false;
    printf("Exiting...");
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
        COUT_( argv[0] << " running");
        return -1;
    }
    // srv srcpass campass
    if(argc!=5)
    {
        //              0               1   2               3           4
        COUT_( argv[0] << "  server server-password cam-password dev/video# ");
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
        int             moinertia=MOTION_INTERTTIA;
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
        frameclient     c(configcam, &q, (const urlinfo*)&u, argv[2]);

        (void)(tickmove);
        ::srand (time(NULL));
        memcpy(&cfg,&_cfg,sizeof(cfg));
        cfg.dirty = false;
        _srvpsw = argv[2];
        _campsw = argv[3];
        _mac = ::macaddr();
        COUT_( "cam token:" << _mac );
        // mac
        _pm = &m;
        c.start_thread();
        COUT_( "MAC: " << _mac );
        while(__alive)
        {
            now = gtc();
            do{
                AutoLock aa(_pm);
                if(_cfg.dirty)
                {
                    COUT_(">>>>>APPLYING CONFIG");
                    c.keep_alive();

                    memcpy(&cfg, &_cfg, sizeof(_cfg));
                    if(_cfg.dirty & CAM_CHANGED)
                    {
                        pdv->close();
                        delete pdv;
                        pdv = new devvideo(&_cfg);
                        __alive = pdv->open();
                        if(!__alive){
                            pdv->close();
                            delete pdv;
                            pdv= nullptr;
                            break;
                        }
                    }
                    _cfg.dirty = 0;
                }
            }while(0);
            sz = 0;
            err = 0;
            const uint8_t*  pb422 = pdv->read(w, h, sz, err);
            if(pb422)
            {
                //COUT_( "got:" << w << "x" << h << ":" << sz <<  "");
                jpgsz=ffmt->convert420(pb422, w, h, sz, cfg.quality, &pjpg);
                //
                // motion
                //
                if((cfg.motionl && cfg.motionh > cfg.motionl) || 
moinertia)
                {
                    movepix = pdv->movement();
                    if(--moinertia>0 || ( movepix >= cfg.motionl && movepix <= cfg.motionh))
                    {
                        bstream = true;
                        cfg.motion = movepix;
                        if( movepix >= cfg.motionl && movepix <= cfg.motionh)
                        {
                            moinertia = MOTION_INTERTTIA;
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
                        COUT_("TLAPSE ON");
                        bstream = true;
                        lapsetick = now;
                    }
                }

                if(cfg.webclienton>0)
                {
                    --cfg.webclienton;
                    bstream = true;
                }

                if(bstream)
                {
                    CDBG("STREAMING ");
                    frame* pf = new frame(true);
                    if(pf)
                    {
                        pf->add((uint8_t*)&jpgsz, sizeof(jpgsz), 0); // 4 bytes length of the frame
                        if(pf->add(pjpg, jpgsz, 0))
                        {
                            //COUT_( "enquing frame " << jpgsz ");
                            q.enque(pf);
                        }
                        else
                        {
                            COUT_( "buffer to small ");
                            exit(2);
                        }
                    }
                    c.keep_alive();
                }
                else {
                    CDBG("NO STREAM");
                }
            }
            else
            {
                CDBG( "getting camera fram 422 format failed");
            }
            bstream  = false;
            ticksave = now;
            int sleep = 1000 / cfg.fps;
            if(sleep < 64) sleep=64;
            ::msleep(sleep);
#ifdef DEBUG
            ::msleep(1000);
#endif
        }
    }
    if(pdv){
        pdv->close();
        delete pdv;
    }
    delete ffmt;
    return 0;
}

void configcam(config* pc)
{
    //strcpy(cloco.acl2, xencrypt(srand, _campsw).c_str());  server
    std::string random = xdecrypt(pc->acl2, _campsw);
    if(pc->rndm == ::atoi(random.c_str()))
    {
        COUT_("CAMERA CONFIG REQUEST");
        AutoLock a(_pm);
        ::memcpy(&_cfg, pc, sizeof(_cfg));
    }
}
