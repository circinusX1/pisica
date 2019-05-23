#include "config.h"

/*
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
int quality
int timelapse;
int darkaverage;
int darkmotion;
int rotate;
int noisereduction; // pix/nr * nr to eliminate noise pixels
int reject[16]; //4 reacts x,y
*/
config Cfg={0,
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
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

};
