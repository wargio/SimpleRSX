/*
 * RSX.cpp
 *
 *  Created on: 11/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#include <SimpleRSX/RSX.h>

#include <malloc.h>
#include <rsx/rsx.h>
#include <sysutil/video.h>
#include <string.h>
#include <unistd.h>

#define CB_SIZE					0x100000
#define HOST_SIZE				(32*1024*1024)
#define GCM_BUFFER_SIZE			2
#define GCM_LABEL_INDEX			255

RSX::RSX() :
		_width(0), _height(0), _resolution(
		VIDEO_RESOLUTION_480), _buffer(NULL) {
	_slabelval = 1;
	_currentid = 0;
	_error = RSX::RSX_OK;

	initializeVideo();
	createBuffers();
	flip();
	setRenderTarget();
	waitFlip();
}

RSX::~RSX() {
	if (_haddr)
		stop();
}

void RSX::stop() {
	rsxFinish(_context, 1);
	for (u32 i = 0; i < GCM_BUFFER_SIZE; ++i) {
		if (_buffer[i].address != NULL)
			rsxFree(_buffer[i].address);
	}
	free(_haddr);
	_haddr = NULL;
}

RSX::rsx_error_codes RSX::errors() {
	return _error;
}

void RSX::initializeVideo() {
	videoState videostate;
	videoConfiguration videoconfig;
	videoResolution resolution;

	/* Allocate a 1Mb buffer, alligned to a 1Mb boundary
	 * to be our shared IO memory with the RSX. */
	_haddr = memalign(1024 * 1024, HOST_SIZE);
	_context = rsxInit(CB_SIZE, HOST_SIZE, _haddr);

	/* Get the state of the display */
	if (videoGetState(0, 0, &videostate) != 0) {
		_error = RSX::RSX_EVIDSTATE;
		return;
	}

	/* Make sure display is enabled */
	if (videostate.state != 0) {
		_error = RSX::RSX_EVIDDISABLED;
		return;
	}

	/* Get the current resolution */
	if (videoGetResolution(videostate.displayMode.resolution, &resolution)
			!= 0) {
		_error = RSX::RSX_EVIDDISABLED;
		return;
	}

	_width = resolution.width;
	_height = resolution.height;

	/* Configure the buffer format to xRGB */
	memset((void*) &videoconfig, 0, sizeof(videoConfiguration));
	videoconfig.resolution = videostate.displayMode.resolution;
	videoconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
	videoconfig.pitch = resolution.width * sizeof(u32);
	videoconfig.aspect = videostate.displayMode.aspect;

	waitIdle();

	if (videoConfigure(0, &videoconfig, NULL, 0) != 0) {
		_error = RSX::RSX_ECONFVID;
		return;
	}

	if (videoGetState(0, 0, &videostate) != 0) {
		_error = RSX::RSX_EVIDSTATE;
		return;
	}

	gcmSetFlipMode(GCM_FLIP_VSYNC); // Wait for VSYNC to flip

	_dpitch = resolution.width * sizeof(u32);
	u32 dsize = (resolution.height * _dpitch) * 2;
	_daddress = (u32*) rsxMemalign(64, dsize);
	rsxAddressToOffset(_daddress, &_doffset);

	gcmResetFlipStatus();
}

void RSX::createBuffers() {
	_buffer = new rsxbuffer[GCM_BUFFER_SIZE];
	u32 depth = sizeof(u32);
	u32 pitch = depth * _width;
	u32 size = pitch * _height;

	for (u32 i = 0; i < GCM_BUFFER_SIZE; ++i) {
		_buffer[i].id = i;
		_buffer[i].address = (uint32_t*) rsxMemalign(64, size);
		if (!_buffer[i].address) {
			_error = RSX::RSX_ENOMEM;
			break;
		}
		if (rsxAddressToOffset(_buffer[i].address, &_buffer[i].offset) != 0) {
			_error = RSX::RSX_EBADOFF;
			break;
		}
		/* Register the display buffer with the RSX */
		if (gcmSetDisplayBuffer(i, _buffer[i].offset, pitch, _width, _height)
				!= 0) {
			_error = RSX::RSX_ESETDISPLAY;
			break;
		}
	}
}

//void RSX::updateresolution() {
//	videoState state;
//	videoResolution resolution;
//
//	/* Get the state of the display */
//	if (videoGetState(0, 0, &state) == 0
//			&& videoGetResolution(state.displayMode.resolution, &resolution)
//					== 0) {
//		_width = resolution.width;
//		_height = resolution.height;
//	} else
//		_height = _width = 0;
//}

u16 RSX::width() {
	return _width;
}

u16 RSX::height() {
	return _height;
}

void RSX::waitFinish(u32 slabelval) {
	rsxSetWriteBackendLabel(_context, GCM_LABEL_INDEX, slabelval);

	rsxFlushBuffer(_context);

	while (*(vu32 *) gcmGetLabelAddress(GCM_LABEL_INDEX) != slabelval)
		usleep(30);

	slabelval++;
}

void RSX::waitIdle() {
	rsxSetWriteBackendLabel(_context, GCM_LABEL_INDEX, _slabelval);
	rsxSetWaitLabel(_context, GCM_LABEL_INDEX, _slabelval);

	_slabelval++;

	waitFinish(_slabelval);
}

void RSX::setRenderTarget() {
	gcmSurface surface;

	surface.colorFormat = GCM_TF_COLOR_X8R8G8B8;
	surface.colorTarget = GCM_TF_TARGET_0;

	surface.colorLocation[0] = GCM_LOCATION_RSX;
	surface.colorLocation[1] = GCM_LOCATION_RSX;
	surface.colorLocation[2] = GCM_LOCATION_RSX;
	surface.colorLocation[3] = GCM_LOCATION_RSX;
	surface.colorOffset[0] = _buffer[_currentid].offset;
	surface.colorOffset[1] = 0;
	surface.colorOffset[2] = 0;
	surface.colorOffset[3] = 0;
	surface.colorPitch[0] = _dpitch;
	surface.colorPitch[1] = 64;
	surface.colorPitch[2] = 64;
	surface.colorPitch[3] = 64;

	surface.depthFormat = GCM_TF_ZETA_Z16;
	surface.depthLocation = GCM_LOCATION_RSX;
	surface.depthOffset = _doffset;
	surface.depthPitch = _dpitch;

	surface.type = GCM_TF_TYPE_LINEAR;
	surface.antiAlias = GCM_TF_CENTER_1;

	surface.width = _width;
	surface.height = _height;
	surface.x = 0;
	surface.y = 0;

	rsxSetSurface(_context, &surface);
}

void RSX::waitFlip() {
	while (gcmGetFlipStatus() != 0)
		usleep(50);

	gcmResetFlipStatus();
}

int RSX::flip() {
	if (gcmSetFlip(_context, _currentid) == 0) {
		rsxFlushBuffer(_context);
		gcmSetWaitFlip(_context);
		_currentid = !_currentid;
		return TRUE;
	}
	return FALSE;
}

gcmContextData* RSX::getGcmCtxData() {
	return _context;
}

u32* RSX::getCurrAddr() {
	return _buffer[_currentid].address;
}

u32 RSX::getCurrOff() {
	return _buffer[_currentid].offset;
}

void RSX::flush(){
	rsxFlushBuffer(_context);
}
