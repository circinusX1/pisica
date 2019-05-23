/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/

    Author:  Marius O. Chincisan
    First Release: September 16 - 29 2016
*/
#ifndef V14LDEVICE_H
#define V14LDEVICE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h> //sudo apt-get install v4l-utils libv4l-dev
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include "mmotion.h"
#include "osthread.h"
#include "config.h"
#include "cimg.h"

#define MAX_BUFFERS 4

class streamq;
struct videobuffer {
    void *  start;
    size_t  length;
    int      mmap;
};


class devvideo
{
public:

    devvideo(const config* cfg);
    virtual ~devvideo();
    bool open();
    void close();
    const uint8_t* read(int& w, int& h, size_t& sz, bool& fatal); // ret 0 fatal, 1 aquired, -1 continue
    const uint8_t* getm(int& w, int& h, size_t& sz); // ret 0 fatal, 1 aquired, -1 continue
    int movement()const{return _moved;}
    uint32_t darkaverage()const{return _pmt ? _pmt->darkav() : 255;}
private:
    int _ioctl(int request, void* argp);
    int _proc_buff(const void* p, struct timeval& t);

private:
    streamq*     _q;
    std::string  _sdevice;
    int         _device;
    int         _xy[2];
    int         _fps;
    int         _curbuffer;
    timeval     _curts;
    uint32_t    _buffsize;
    struct    videobuffer  _buffers[MAX_BUFFERS];
    time_t      _lasttime;
    int         _motionlow;
    int         _motionhi;
    mmotion*    _pmt;
    int         _moved;
    int         _nr;
    bool        _fatal;
};

#endif // V4LDEVICE_H
