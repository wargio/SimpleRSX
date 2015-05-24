/* Now double buffered with animation.
 */

#include <ppu-lv2.h>
#include <stdio.h>
#include <io/pad.h>
#include <rsx/gcm_sys.h>
#include <SimpleRSX/RSX.h>
#include <SimpleRSX/Font.h>
#include <SimpleRSX/Image.h>
#include <SimpleRSX/Debug.h>
#include "image_png.h"

#include <time.h>

#define DEBUG_IP 					"192.168.0.4"
#define DEBUG_PORT 					18194
#define ICON0PATH					"/dev_hdd0/game/SIMPRSXEX/psl1ght.png"

int main(s32 argc, const char* argv[]) {
	u32 frame = 0;
	static time_t starttime = 0;
	double fps = 0;

	Debug dbg(DEBUG_PORT, DEBUG_IP);
	dbg.Printf("RSX\n");
	RSX rsx;
	gcmContextData* context = rsx.getGcmCtxData();

	Image mem(&rsx);
	Image fp(&rsx);
	Font latin(&rsx);
	Font latinYellow(&rsx, 0xFFFFFF00);
	if (latin.open(Font::PS3_VR_R_LATIN2, 120u) != Font::FONT_OK)
		goto end;
	if (latinYellow.open(Font::PS3_VR_R_LATIN2, 72u) != Font::FONT_OK)
		goto end;

	if (fp.open(ICON0PATH) != Image::IMAGE_OK)
		goto end;
	if (mem.open((void*) image_png, image_png_size, Image::IMAGE_PNG_TYPE))
		goto end;

	padInfo padinfo;
	padData paddata;
	ioPadInit(7);

	char sfps[128];

	while (1) {
		if (starttime == 0)
			starttime = time(NULL);
		else
			fps = frame / difftime(time(NULL), starttime);
		// Check the pads.
		ioPadGetInfo(&padinfo);
		if (padinfo.status[0]) {
			ioPadGetData(0, &paddata);
			if (paddata.BTN_START) {
				goto end;
			}
		}

		/* tells the GPU to show the frame */
		rsx.flip();
		/* setting the new render target */
		rsx.setRenderTarget();
		/* waiting the frame to be seen */
		rsx.waitFlip();
		/* now i can write into the buffer */
		rsxSetClearColor(context, 0x200030);
		rsxSetClearDepthValue(context, 0xffff);
		rsxClearSurface(context, GCM_CLEAR_R |
		GCM_CLEAR_G |
		GCM_CLEAR_B |
		GCM_CLEAR_A |
		GCM_CLEAR_S |
		GCM_CLEAR_Z);
		fp.copy(1, 1);
		mem.alpha(550, 650);
		snprintf(sfps, 128, "SimpleRSX %dx%d FPS %.2f", rsx.width(),
				rsx.height(), fps);
		latinYellow.alphaPrint("latinYellow!!", 700, 200);
		latin.copyPrint("latin!!", 700, 500);
		latinYellow.alphaPrint(sfps, 100, 400);
		++frame;
	}

	end: // stopping

	fp.clear();
	mem.clear();

	ioPadEnd();
	rsx.stop();
	dbg.Printf("bye\n");

	return 0;
}
