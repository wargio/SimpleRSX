/*
 * RSX.h
 *
 *  Created on: 11/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#ifndef RSX_H_
#define RSX_H_

#include <ppu-lv2.h>
#include <rsx/rsx.h>

class RSX {
public:
	typedef enum _rsx_error_codes {
		RSX_OK = 0,
		RSX_EVIDSTATE,
		RSX_ECONFVID,
		RSX_EVIDDISABLED,
		RSX_ENOMEM,
		RSX_EBADOFF,
		RSX_ESETDISPLAY,
		RSX_EBADSTATE
	} rsx_error_codes;

	RSX();
	virtual ~RSX();

	rsx_error_codes errors();

	u16 width();
	u16 height();


	gcmContextData* getGcmCtxData();
	u32* getCurrAddr();
	u32 getCurrOff();

	void stop();

	void flush();
	int  flip();
	void waitFlip();
	void setRenderTarget();

protected:
	void initializeVideo();
	void createBuffers();

	void waitIdle();
	void waitFinish(u32 slabelval);


private:
	typedef struct _rsxbuffer {
		u32 id;
		u32 offset;
		u32* address;
	} rsxbuffer;

	rsx_error_codes _error;
	u16 _width;
	u16 _height;
	u32 _dpitch;
	u32 _doffset;
	u32* _daddress;
	u16 _currentid;
	u16 _resolution;
	u32 _slabelval;
	void* _haddr;
	rsxbuffer* _buffer;
	gcmContextData* _context;
};

#endif /* RSX_H_ */
