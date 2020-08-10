#ifndef CONFIG_H
#define CONFIG_H

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
    uint8_t dirty;
    char url[128];
    char device[64];
    int  w;
    int  h;
    int  fps;
    int  motionl;
    int  motionh;
    int  motionw;
    int  motiondiff;
    int  motionsnap;
    int quality;
    int timelapse;
    int darkaverage;
    int darkmotion;
    int rotate;
    int noisereduction; // pix/nr * nr to eliminate noise pixels
    int streame;
    int reject[16]; //4 reacts x,y
};

extern config Cfg;

inline time_t gtc(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return time_t(now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0);
}


#endif // CONFIG_H
