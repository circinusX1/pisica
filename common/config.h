
#ifndef CONFIGX_H
#define CONFIGX_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

struct __attribute__((__packed__)) rrect
{
    uint8_t xm;
    uint8_t ym;
    uint8_t xM;
    uint8_t yM;
};
#define PROPS_CHANGED       1
#define CAM_CHANGED         0x80

#ifdef DEBUG
#   define WEB_CLION_FRAMES    16
#else
#   define WEB_CLION_FRAMES    256
#endif
struct __attribute__((__packed__)) config
{
    char url[64];
    char device[24];
    char acl[24];
    char acl1[24];
    char acl2[24];
    char mac[24];
    uint8_t motionrejectrects[16]; // 4 rects to reject rectangles in motion up to motionw/motionh
    uint8_t dirty;
    int  rndm;
    int  w;
    int  h;
    uint8_t  fps;
    int     motionl;  // motion pixels low
    int     motionh;  // motion pixels high
    uint8_t     motionw;  // motion width pix. height is calculated
    int     motiondiff; // prevfram -curframe
    uint8_t  quality;   // jpg 80
    uint8_t  timelapse;
    uint8_t  darkaverage;
    uint8_t  darkmotion;
    uint8_t  rotate;
    uint8_t  noisereduction; // pix/nr * nr to eliminate noise pixels
    uint32_t  webclienton;
    bool  motion;
    char  filler[11]; // keep 256 bytes though
    unsigned int   sig;
};

extern config _cfg;

inline time_t gtc(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return time_t(now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0);
}


#define LOCK_(m,i)      do{\
        AutoLock l(m);     \
        i;                 \
    }while(0);


#define COUT_(x_)  std::cout << x_<< "\n"
#define DERR(x_) std::cerr << x_ << errno << "\n"
#ifdef DEBUG
#define CDBG(x_) std::cout << x_<< "\n"
#else
#define CDBG(x_) (void)(x_)
#endif

#endif // CONFIG_H
