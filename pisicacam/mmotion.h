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
#ifndef MOTIONNN_H
#define MOTIONNN_H

#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include "mmotion.h"
#include "osthread.h"
#include "config.h"


class mmotion
{
public:
    mmotion(int w, int h, int nr);
    ~mmotion();
    int has_moved(uint8_t* p);
    int  getw()const{return _mw;}
    int  geth()const{return _mh;}
    uint8_t*  motionbuf()const{return _motionbufs[2];}
    uint32_t darkav()const{return _dark;}

private:
    int       _w;
    int       _h;
    int       _mw;
    int       _mh;
    uint8_t*  _motionbufs[3];
    int       _motionindex;
    uint32_t  _motionsz;
    umutex    _m;
    int       _moves;
    uint32_t  _dark;
    int       _nr;
    int       _mmeter;

};



#endif // V4LDEVICE_H
