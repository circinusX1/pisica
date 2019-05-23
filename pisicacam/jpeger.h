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
#ifndef JPEGER_H
#define JPEGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include "ffmt.h"
#include <time.h>
#include <jpeglib.h>
#include <jerror.h>

class jpeger : public outfilefmt
{
public:
    jpeger(int q);
    virtual ~jpeger();
    uint32_t convert420(const uint8_t* fmt420, int w ,int h, int isize, int quality, uint8_t** pjpeg);
    uint32_t convertBW(const uint8_t* uint8buf, int w, int h, int isize,
                           int quality, uint8_t** pjpeg);

private:
    int _put_jpeg_yuv420p_memory(uint8_t *dest_image, int image_size,
                   const uint8_t *input_image, int width, int height, int quality,
                   struct tm *tm);

    void _jpeg_mem_dest(j_compress_ptr cinfo, JOCTET* buf, size_t bufsize);
private:
    uint8_t*    _image;
    int         _jpegQuality;
    uint32_t    _imgsize;
    int         _memsz;
};

#endif // JPEGER_H
