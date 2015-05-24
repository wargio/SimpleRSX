/*
 * Font.cpp
 *
 *  Created on: 12/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#include <SimpleRSX/Font.h>
#include <cstring>
#include <rsx/mm.h>
#include <rsx/rsx.h>

Font::Font(RSX* rsx) :
		_rsx(rsx) {
	_color = 0;
	for (u32 i = 0; i < 256; ++i)
		glyph[i].data = NULL;
}

Font::Font(RSX* rsx, u32 argb) :
		_rsx(rsx) {
	_color = argb;
	for (u32 i = 0; i < 256; ++i)
		glyph[i].data = NULL;
}

Font::~Font() {
	for (u32 i = 0; i < 256; ++i)
		if (glyph[i].data)
			rsxFree(glyph[i].data);
}

Font::fontError_t Font::open(fontps3_t id, u32 fontSize) {

	const char* _internal_fonts[] = {
			"/dev_flash/data/font/SCE-PS3-NR-R-JPN.TTF",
			"/dev_flash/data/font/SCE-PS3-YG-R-KOR.TTF",
			"/dev_flash/data/font/SCE-PS3-DH-R-CGB.TTF",
			"/dev_flash/data/font/SCE-PS3-CP-R-KANA.TTF",
			"/dev_flash/data/font/SCE-PS3-VR-R-LATIN2.TTF",
			NULL };

	const char* path = _internal_fonts[id];
	if (!path)
		return FONT_BAD_ID;
	return open(path, fontSize);
}

Font::fontError_t Font::open(const char* filename, u32 fontSize) {
	FT_Library library;
	FT_Face face;
	int error = FT_Init_FreeType(&library);
	if (error)
		return FONT_BAD_LIB_INIT;

	error = FT_New_Face(library, filename, 0, &face);
	if (error == FT_Err_Unknown_File_Format)
		return FONT_FILE_NOT_FOUND;
	else if (error)
		return FONT_BAD_FORMAT;

	fontError_t ferror = fillGlyph(library, face, fontSize);
	if (ferror) {
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return ferror;
	}

	return FONT_OK;
}

Font::fontError_t Font::open(const void* buffer, u32 size, u32 fontSize) {
	FT_Library library;
	FT_Face face;
	int error = FT_Init_FreeType(&library);
	if (error)
		return FONT_BAD_LIB_INIT;

	error = FT_New_Memory_Face(library, (u8*) buffer, size, 0, &face);
	if (error)
		return FONT_BAD_FORMAT;

	fontError_t ferror = fillGlyph(library, face, fontSize);
	if (ferror) {
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return ferror;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return FONT_OK;
}

Font::fontError_t Font::fillGlyph(FT_Library& library, FT_Face& face,
		u32& fontSize) {
	int error = FT_Set_Char_Size(face, 0, fontSize * 64, 72, 72);
	if (error)
		return FONT_BAD_SIZE;

	error = FT_Select_Charmap(face, FT_ENCODING_ADOBE_CUSTOM);
	if (error) {
		error = FT_Select_Charmap(face, FT_ENCODING_ADOBE_STANDARD);
		if (error) {
			error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
			if (error)
				return FONT_BAD_ENC;
		}
	}
	{
		FT_UInt index;
		u32 size;
		u16 i = (u16) ' ';
		FT_Bitmap* bitmap;
		FT_GlyphSlot slot;
		u32 c;
		while (i < 0x7F) {
			index = FT_Get_Char_Index(face, i);
			error = FT_Load_Glyph(face, index, FT_LOAD_RENDER);
			if (error) {
				return FONT_BAD_GLYPH;
			}
			slot = face->glyph;
			error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
			if (error) {
				return FONT_BAD_RENDER;
			}
			bitmap = &slot->bitmap;
			glyph[i].width = bitmap->width;
			glyph[i].height = bitmap->rows;
			glyph[i].width += (glyph[i].width % 2);
			glyph[i].height += (glyph[i].height % 2);
			size = glyph[i].width * glyph[i].height * sizeof(u32);
			if (size > 0) {
				glyph[i].data = (u32*) rsxMemalign(128, size);
				memset(glyph[i].data, 0, size);
				if(_color == 0){
					for (u32 j = 0, p = 0, b = 0; j < (u32) bitmap->rows; ++j) {
						p = glyph[i].width * j;
						b = bitmap->width * j;
						for (u32 k = 0; k < (u32) bitmap->width; ++k) {
							c = bitmap->buffer[b + k];
							glyph[i].data[p + k] = (u32) (c * 0x01010101); // & etc..
						}
					}					
				} else {
					for (u32 j = 0, p = 0, b = 0; j < (u32) bitmap->rows; ++j) {
						p = glyph[i].width * j;
						b = bitmap->width * j;
						for (u32 k = 0; k < (u32) bitmap->width; ++k) {
							c = bitmap->buffer[b + k];
							glyph[i].data[p + k] = (u32) (c * 0x01010101); // & etc..
							glyph[i].data[p + k] = glyph[i].data[p + k] & _color;
						}
					}					

				}
			} else
				glyph[i].data = NULL;
			glyph[i].left = slot->bitmap_left;
			glyph[i].top = slot->bitmap_top;
			glyph[i].x = slot->advance.x >> 6;
			glyph[i].y = slot->advance.y >> 6;
			++i;
		}
	}
	return FONT_OK;
}

Font::fontError_t Font::alphaPrint(const char* message, u16 x, u16 y) {
	u32 len = strlen(message);
	char c;
	for (u32 i = 0; i < len; ++i) {
		c = message[i];
		alpha(x, y, c);
		x += glyph[(u32) c].x;
		y += glyph[(u32) c].y;

	}
	return FONT_OK;
}

Font::fontError_t Font::copyPrint(const char* message, u16 x, u16 y) {
	u32 len = strlen(message);
	char c;
	for (u32 i = 0; i < len; ++i) {
		c = message[i];
		copy(x, y, c);
		x += glyph[(u32) c].x;
		y += glyph[(u32) c].y;
	}
	return FONT_OK;
}

void Font::alpha(u32 dstX, u32 dstY, char c) {
	if (c < ' ' || c > '~' || !glyph[(u32) c].data)
		return;
	u32 offset;
	rsxAddressToOffset(glyph[(u32) c].data, &offset);
	gcmContextData* ctx = _rsx->getGcmCtxData();
	rsxSetBlendFunc(ctx, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA,
	GCM_ONE_MINUS_SRC_ALPHA);
	rsxSetBlendEquation(ctx, GCM_FUNC_ADD, GCM_FUNC_ADD);
	rsxSetBlendEnable(ctx, GCM_TRUE);

	gcmTransferScale scale;
	gcmTransferSurface surface;

	u16 sW = glyph[(u32) c].width;
	u16 sH = glyph[(u32) c].height;

	scale.conversion = GCM_TRANSFER_CONVERSION_TRUNCATE;
	scale.format = GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
	scale.origin = GCM_TRANSFER_ORIGIN_CORNER;
	scale.operation = GCM_TRANSFER_OPERATION_SRCCOPY_AND;
	scale.interp = GCM_TRANSFER_INTERPOLATOR_NEAREST;
	scale.clipX = 0;
	scale.clipY = 0;
	scale.clipW = _rsx->width();
	scale.clipH = _rsx->height();
	scale.outX = dstX + glyph[(u32) c].left;
	scale.outY = dstY - glyph[(u32) c].top;
	scale.outW = sW;
	scale.outH = sH;
	scale.ratioX = rsxGetFixedSint32(1.f);
	scale.ratioY = rsxGetFixedSint32(1.f);
	scale.inX = rsxGetFixedUint16(0);
	scale.inY = rsxGetFixedUint16(0);
	scale.inW = sW;
	scale.inH = sH;
	scale.offset = offset;
	scale.pitch = sizeof(u32) * glyph[(u32) c].width;
	surface.format = GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
	surface.pitch = _rsx->width() * sizeof(u32);
	surface.offset = _rsx->getCurrOff();

	rsxSetTransferScaleMode(ctx, GCM_TRANSFER_LOCAL_TO_LOCAL,
	GCM_TRANSFER_SURFACE);
	rsxSetTransferScaleSurface(ctx, &scale, &surface);
}

void Font::copy(u32 dstX, u32 dstY, char c) {
	if (c < ' ' || c > '~' || !glyph[(u32) c].data)
		return;
	u32 offset;
	rsxAddressToOffset(glyph[(u32) c].data, &offset);
	rsxSetTransferImage(_rsx->getGcmCtxData(), GCM_TRANSFER_LOCAL_TO_LOCAL,
			_rsx->getCurrOff(), _rsx->width() * sizeof(u32),
			dstX + glyph[(u32) c].left,
			dstY - glyph[(u32) c].top, offset,
			glyph[(u32) c].width * 4, 0, 0, glyph[(u32) c].width,
			glyph[(u32) c].height, 4);
}
