/*
 * Image.cpp
 *
 *  Created on: 11/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#include <SimpleRSX/Image.h>
#include <cstring>
#include <rsx/mm.h>
#include <jpgdec/jpgdec.h>
#include <pngdec/pngdec.h>
#include <sysmodule/sysmodule.h>
#include <cctype>

Image::Image(RSX* rsx) :
		_rsx(rsx), _data(NULL), _width(0), _height(0) {
	if (sysModuleIsLoaded(SYSMODULE_PNGDEC))
		sysModuleLoad(SYSMODULE_PNGDEC);
	if (sysModuleIsLoaded(SYSMODULE_JPGDEC))
		sysModuleLoad(SYSMODULE_JPGDEC);
}

Image::~Image() {
	clear();
	if (!sysModuleIsLoaded(SYSMODULE_PNGDEC))
		sysModuleLoad(SYSMODULE_PNGDEC);
	if (!sysModuleIsLoaded(SYSMODULE_JPGDEC))
		sysModuleLoad(SYSMODULE_JPGDEC);
}

Image::imgstatus_t Image::open(const void* buffer, u32 size, imgtype_t type) {
	if (!buffer || !size)
		return IMAGE_BAD_ARG;

	if (_data)
		clear();

	int ret = 0;
	switch (type) {
	case Image::IMAGE_JPEG_TYPE: {
		jpgData j;
		ret = jpgLoadFromBuffer(buffer, size, &j);
		if (ret) {
			return IMAGE_UNKN_FORMAT;
		}
		Image::imgstatus_t err = jpegMoveToRSX((void*) &j);
		if (err) {
			return err;
		}
	}
		break;
	case Image::IMAGE_PNG_TYPE: {
		pngData j;
		ret = pngLoadFromBuffer(buffer, size, &j);
		if (ret) {
			return IMAGE_UNKN_FORMAT;
		}
		Image::imgstatus_t err = jpegMoveToRSX((void*) &j);
		if (err) {
			return err;
		}
	}
		break;
	default:
		return IMAGE_UNKN_FORMAT;
	}
	if (rsxAddressToOffset(_data, &_offset))
		return IMAGE_BAD_OFFSET;

	return IMAGE_OK;
}

Image::imgstatus_t Image::open(const char* filepath) {
	if (!filepath)
		return IMAGE_BAD_ARG;

	if (_data)
		clear();

	u32 size = strlen(filepath);
	char extension[5] = { (char) tolower(filepath[size - 4]), (char) tolower(
			filepath[size - 3]), (char) tolower(filepath[size - 2]),
			(char) tolower(filepath[size - 1]), '\0' };

	int ret = 0;
	if (strncmp(extension, "jpeg", 4) == 0
			|| strncmp(extension, ".jpg", 4) == 0) {
		jpgData j;
		ret = jpgLoadFromFile(filepath, &j);
		if (ret) {
			return IMAGE_UNKN_FORMAT;
		}
		Image::imgstatus_t err = jpegMoveToRSX((void*) &j);
		if (err) {
			return err;
		}
	} else if (strncmp(extension, ".png", 4) == 0) {
		pngData j;
		ret = pngLoadFromFile(filepath, &j);
		if (ret) {
			return IMAGE_UNKN_FORMAT;
		}
		Image::imgstatus_t err = jpegMoveToRSX((void*) &j);
		if (err) {
			return err;
		}
	}
	if (rsxAddressToOffset(_data, &_offset))
		return IMAGE_BAD_OFFSET;
	return IMAGE_OK;
}

Image::imgstatus_t Image::clear() {
	if (_data) {
		rsxFree(_data);
		_data = NULL;
		_pitch = _width = _height = 0;
		return IMAGE_OK;
	}
	return IMAGE_BAD_FREE;
}

u32 Image::width() {
	return _width;
}

u32 Image::height() {
	return _height;
}

u32 Image::pitch() {
	return _pitch;
}

void Image::alpha(u32 dstX, u32 dstY) {
	if (!_data)
		return;
	gcmContextData* ctx = _rsx->getGcmCtxData();
	rsxSetBlendFunc(ctx, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA,
	GCM_ONE_MINUS_SRC_ALPHA);
	rsxSetBlendEquation(ctx, GCM_FUNC_ADD, GCM_FUNC_ADD);
	rsxSetBlendEnable(ctx, GCM_TRUE);

	gcmTransferScale scale;
	gcmTransferSurface surface;

	scale.conversion = GCM_TRANSFER_CONVERSION_TRUNCATE;
	scale.format = GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
	scale.origin = GCM_TRANSFER_ORIGIN_CORNER;
	scale.operation = GCM_TRANSFER_OPERATION_SRCCOPY_AND;
	scale.interp = GCM_TRANSFER_INTERPOLATOR_NEAREST;
	scale.clipX = 0;
	scale.clipY = 0;
	scale.clipW = _rsx->width();
	scale.clipH = _rsx->height();
	scale.outX = dstX;
	scale.outY = dstY;
	scale.outW = _width;
	scale.outH = _height;
	scale.ratioX = rsxGetFixedSint32(1.f);
	scale.ratioY = rsxGetFixedSint32(1.f);
	scale.inX = rsxGetFixedUint16(0);
	scale.inY = rsxGetFixedUint16(0);
	scale.inW = _width;
	scale.inH = _height;
	scale.offset = _offset;
	scale.pitch = sizeof(u32) * _width;

	surface.format = GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
	surface.pitch = _rsx->width() * sizeof(u32);
	surface.offset = _rsx->getCurrOff();

	rsxSetTransferScaleMode(ctx, GCM_TRANSFER_LOCAL_TO_LOCAL,
	GCM_TRANSFER_SURFACE);

	rsxSetTransferScaleSurface(ctx, &scale, &surface);
}

void Image::copy(u32 dstX, u32 dstY) {
	if (!_data)
		return;
	rsxSetTransferImage(_rsx->getGcmCtxData(), GCM_TRANSFER_LOCAL_TO_LOCAL,
			_rsx->getCurrOff(), _rsx->width() * sizeof(u32), dstX,
			dstY, _offset, _width * 4, 0, 0, _width, _height, 4);
}

Image::imgstatus_t Image::jpegMoveToRSX(void* t) {
	jpgData* d = (jpgData*) t;
	u32 size = d->width * d->height * 4;
	_data = (u32*) rsxMemalign(128, size);
	if (!_data)
		return IMAGE_NOMEM;

	memcpy(_data, d->bmp_out, size);
	free(d->bmp_out);

	_width = d->width;
	_height = d->height;
	_pitch = d->pitch;
	return IMAGE_OK;
}
Image::imgstatus_t Image::pngMoveToRSX(void* t) {
	pngData* d = (pngData*) t;
	u32 size = d->width * d->height * 4;
	_data = (u32*) rsxMemalign(128, size);
	if (!_data)
		return IMAGE_NOMEM;

	memcpy(_data, d->bmp_out, size);
	free(d->bmp_out);

	_width = d->width;
	_height = d->height;
	_pitch = d->pitch;
	return IMAGE_OK;
}
