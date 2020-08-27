
#ifndef CONFIGX_H
#define CONFIGX_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

struct __attribute__((__packed__)) rrect
{
    int xm;
    int ym;
    int xM;
    int yM;
};


struct __attribute__((__packed__)) config
{
    char url[64];
    char device[32];
    char acl[32];
    char acl1[32];
    char acl2[32];
    char mac[16];
    char reject[16]; //4 reacts x,y
    bool dirty;
    int  rndm;
    int  w;
    int  h;
    uint8_t  fps;
    int     motionl;
    int     motionh;
    int     motionw;
    int     motiondiff;
    int     motionsnap;
    uint8_t  quality;
    uint8_t  timelapse;
    uint8_t  darkaverage;
    uint8_t  darkmotion;
    uint8_t  rotate;
    uint8_t  noisereduction; // pix/nr * nr to eliminate noise pixels
    uint8_t  streame;
    uint8_t client;
    uint8_t lapse;
    bool  motion;
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


#endif // CONFIG_H
