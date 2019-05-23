#include "skimg.h"

/**
 * @brief JpegHdr  (multipart animated JPEG)
 */
static char JpegHdr[]="Content-Type: image/jpeg\r\n"
                       "Content-Length: %d\r\n"
                       "X-Timestamp: %d.%d\r\n\r\n";

static char JpegPart[] = "\r\n--tdddupQTZexunixxxxyyyy\r\nContent-type: image/jpeg\r\n";

static char JpegPartHeader[] = "HTTP/1.0 200 OK\r\n"
                               "Connection: close\r\n"
                               "Server: pisica/1.0\r\n"
                               "Cache-Control: no-cache\r\n"
                               "Content-Type: multipart/x-mixed-replace;boundary=tdddupQTZexunixxxx\r\n"
                               "\r\n"
                               "--tdddupQTZexunixxxxyyyy\r\n";
skimg::skimg(skbase& o):skweb(o)
{
}

skimg::~skimg()
{
}

int skimg::snd(const uint8_t* b, size_t len,uint32_t extra)
{
    char    buffer[512] = {0};
    struct timeval timestamp;
    struct timezone tz = {5,0};

    gettimeofday(&timestamp, &tz);

    if(_hok==false)
    {
        skweb::snd((const uint8_t*)JpegPartHeader, strlen(JpegPartHeader));
        _hok=true;
    }
    size_t hl = ::sprintf(buffer, JpegHdr,
                                    (int)len,
                                    (int)timestamp.tv_sec,
                                    (int)timestamp.tv_usec);
    skweb::snd((const uint8_t*)buffer,hl);
    skweb::snd(b,len);
    skweb::snd((const uint8_t*)JpegPart,strlen(JpegPart));
    return 1;
}

