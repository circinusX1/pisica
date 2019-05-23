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
#include <iostream>
#include "jpeger.h"

extern bool __alive;

typedef struct
{
    struct jpeg_destination_mgr pub;
    JOCTET *buf;
    size_t bufsize;
    size_t jpegsize;
} mem_destination_mgr;

typedef mem_destination_mgr *mem_dest_ptr;
static void     _init_destination(j_compress_ptr cinfo);
static boolean  _empty_output_buffer(j_compress_ptr cinfo);
static void     _term_destination(j_compress_ptr cinfo);
static int      _jpeg_mem_size(j_compress_ptr cinfo);

jpeger::jpeger(int q):_image(0),_jpegQuality(q),_imgsize(0),_memsz(0)
{
}

jpeger::~jpeger()
{
    delete[] _image;//dtor
}

uint32_t jpeger::convert420(const uint8_t* fmt420, int w, int h, int isize,
                            int quality, uint8_t** pjpeg)
{
    if(_image==nullptr)
    {
        _memsz = w * h * 5;
        _image = new uint8_t[_memsz]; // this should be enough ?!?
    }
    if(_image == nullptr)
    {
        std::cerr <<  "out of memory" << DERR();
        __alive=false;
        return 0;
    }
   // std::cout <<"img=" << std::hex << (size_t)_image << std::dec << " pointer \n";
    _imgsize =  _put_jpeg_yuv420p_memory(_image, isize, fmt420, w, h, quality, 0);
    *pjpeg = _image;
    return  _imgsize;
}


uint32_t jpeger::convertBW(const uint8_t* uint8buf, int w, int h, int imgsz,
                           int quality, uint8_t** pjpeg)
{

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int row_stride;

    memset(_image,255, _memsz);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);

    jpeg_set_colorspace(&cinfo, JCS_GRAYSCALE);
    //cinfo.raw_data_in = TRUE;
    cinfo.dct_method = JDCT_FASTEST;

    jpeg_set_quality(&cinfo, 80, TRUE /* limit to baseline-JPEG values */);
    _jpeg_mem_dest(&cinfo, _image, imgsz);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = w ;


    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = (unsigned char*)&uint8buf[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    int jpeg_imgsz = _jpeg_mem_size(&cinfo);
    *pjpeg = _image;
    jpeg_destroy_compress(&cinfo);
    return jpeg_imgsz;
}

int jpeger::_put_jpeg_yuv420p_memory(uint8_t *pdest,
                                     int imgsz,
                                     const uint8_t *pyuv420,
                                     int width,
                                     int height,
                                     int quality,
                                     struct tm *tm)
{
    int i, j, jpeg_imgsz;
    JSAMPROW y[16],cb[16],cr[16];
    JSAMPARRAY data[3];
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    data[0] = y;
    data[1] = cb;
    data[2] = cr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    jpeg_set_defaults(&cinfo);
    jpeg_set_colorspace(&cinfo, JCS_YCbCr);
    cinfo.raw_data_in = TRUE;
#if JPEG_LIB_VERSION >= 70
    cinfo.do_fancy_downsampling = FALSE;
#endif
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;

    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.dct_method = JDCT_FASTEST;
    _jpeg_mem_dest(&cinfo, pdest, imgsz);
    jpeg_start_compress(&cinfo, TRUE);

    for (j = 0; j < height; j += 16)
    {
        for (i = 0; i < 16; i++)
        {
            y[i] = ( unsigned char*)pyuv420 + width * (i + j);

            if (i % 2 == 0)
            {
                cb[i / 2] = ( unsigned char*)pyuv420 + width * height + width / 2 * ((i + j) /2);
                cr[i / 2] = ( unsigned char*)pyuv420 + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
            }
        }
        jpeg_write_raw_data(&cinfo, data, 16);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_imgsz = _jpeg_mem_size(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return jpeg_imgsz;
}

void jpeger:: _jpeg_mem_dest(j_compress_ptr cinfo, JOCTET* buf, size_t bufsize)
{
    mem_dest_ptr dest;

    if (cinfo->dest == NULL)
    {
        cinfo->dest = (struct jpeg_destination_mgr *)
                      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
                              sizeof(mem_destination_mgr));
    }
    dest = (mem_dest_ptr) cinfo->dest;
    dest->pub.init_destination    = _init_destination;
    dest->pub.empty_output_buffer = _empty_output_buffer;
    dest->pub.term_destination    = _term_destination;
    dest->buf      = buf;
    dest->bufsize  = bufsize;
    dest->jpegsize = 0;
}

static void  _init_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    dest->pub.next_output_byte = dest->buf;
    dest->pub.free_in_buffer = dest->bufsize;
    dest->jpegsize = 0;
}

static boolean  _empty_output_buffer(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    dest->pub.next_output_byte = dest->buf;
    dest->pub.free_in_buffer = dest->bufsize;
    return FALSE;
    ERREXIT(cinfo, JERR_BUFFER_SIZE);
}

static void _term_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    dest->jpegsize = dest->bufsize - dest->pub.free_in_buffer;
}

static int _jpeg_mem_size(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    return dest->jpegsize;
}
