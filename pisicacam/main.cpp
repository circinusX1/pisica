
#include "main.h"
#include "frameclient.h"
#include "singleton.h"
#include "devvideo.h"
#include "../common/config.h"
#include "ffmt.h"
#include "jpeger.h"
#include "vigenere.h"

bool                __alive = true;
static umutex*      _pm;
static bool         _dconf = false;
static std::string  _sp;// = argv[1];
static std::string  _pp;// = argv[1];
static std::string  _tok;// = argv[1];
std::string         _mac;
config       _cfg = {0,
                         "http://localhost:8888",
                         "/dev/video0",
                         "",
                         640,
                         480,
                         30,
                         50,
                         800,
                         100,
                         10,
                         1000,
                         80,
                         1,
                         10,
                         100,
                         0,
                         4,
                         1,
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         1,
                        1,
                        0
             };

void setts(config* pc);


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
    if(argc!=6)
    {
        std::cout << argv[0] << "  server server-password cam-password cam-token dev/video# \n";
        std::cout << "\n";
        exit(1);
    }

    outfilefmt*     ffmt = new jpeger(_cfg.quality);
    urlinfo u(argv[1]);
    streamq q;

    ::strcpy(_cfg.device,argv[5]);
    devvideo* pdv = new devvideo(&_cfg);
    if(pdv->open())
    {
        config          cfg = {0};
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
        frameclient     c(setts, &q, (const urlinfo*)&u, argv[2]);

        _sp = argv[2];
        _pp = argv[3];
        _tok = argv[4];
        ::strcpy(_cfg.authplay,xencrypt(_tok,_pp).c_str());
        _mac = ::macaddr();
        _pm = &m;
        c.start_thread();
        std::cout << "MAC: " << _mac << "\n";
        while(__alive)
        {
            now = gtc();
            do{
                AutoLock a(_pm);
                if(_dconf)
                {
                    ::memcpy(&cfg, &_cfg, sizeof(cfg));
                    _dconf = false;
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
		        //std::cout << "got:" << w << "x" << h << ":" << sz << 
"\n";
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
		    else
	 	    {
			std::cout << "new frame failed\n";
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
    ::strcpy(_cfg.authplay,xencrypt(_tok,_pp).c_str());
   _dconf=true;
}
