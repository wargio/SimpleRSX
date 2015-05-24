/*
 * Image.h
 *
 *  Created on: 11/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#include <SimpleRSX/RSX.h>
#include <ppu-lv2.h>

class Image {
public:
	typedef enum _imgtype_t {
		IMAGE_JPEG_TYPE = 0, IMAGE_PNG_TYPE
	} imgtype_t;

	typedef enum _imgstatus_t {
		IMAGE_OK = 0,
		IMAGE_BAD_ARG,
		IMAGE_BAD_FREE,
		IMAGE_BAD_OFFSET,
		IMAGE_NOMEM,
		IMAGE_UNKN_FORMAT
	} imgstatus_t;

	Image(RSX* rsx);
	virtual ~Image();

	imgstatus_t open(const void* buffer, u32 size, imgtype_t type);
	imgstatus_t open(const char* filepath);
	imgstatus_t clear();

	u32 width();
	u32 height();
	u32 pitch();

	void alpha(u32 dstX, u32 dstY);
	void copy(u32 dstX, u32 dstY);

private:
	Image::imgstatus_t jpegMoveToRSX(void* d);
	Image::imgstatus_t pngMoveToRSX(void* d);

	RSX* _rsx;
	u32* _data;
	u32 _offset;
	u32 _pitch;
	u32 _width;
	u32 _height;
};

#endif /* IMAGE_H_ */
