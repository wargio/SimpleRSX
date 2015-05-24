/*
 * Font.h
 *
 *  Created on: 12/apr/2015
 *      Author: Giovanni Dante Grazioli
 */

#ifndef FONT_H_
#define FONT_H_

#include <SimpleRSX/RSX.h>
#include <freetype/config/ftheader.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H


class Font {
public:
	typedef enum _fontps3_t {
		PS3_NR_R_JPN = 0,
		PS3_YG_R_KOR,
		PS3_DH_R_CGB,
		PS3_CP_R_KANA,
		PS3_VR_R_LATIN2
	} fontps3_t;

	typedef enum _fontError_t {
		FONT_OK = 0,
		FONT_BAD_LIB_INIT,
		FONT_BAD_ID,
		FONT_BAD_FORMAT,
		FONT_BAD_SIZE,
		FONT_BAD_GLYPH,
		FONT_BAD_RENDER,
		FONT_NOMEM,
		FONT_BAD_ENC,
		FONT_FILE_NOT_FOUND
	} fontError_t;

	Font(RSX* rsx);
	Font(RSX* rsx, u32 argb);
	virtual ~Font();

	fontError_t open(fontps3_t id, u32 fontSize);
	fontError_t open(const char* filename, u32 fontSize);
	fontError_t open(const void* buffer, u32 size, u32 fontSize);
	fontError_t alphaPrint(const char* message, u16 x, u16 y);
	fontError_t copyPrint(const char* message, u16 x, u16 y);

private:
	typedef struct _glyph_data_t {
		u32* data;
		u16 width;
		u16 height;
		short top;
		short left;
		short x;
		short y;
	} glyphData_t;

	fontError_t fillGlyph(FT_Library& library, FT_Face& face, u32& fontSize);
	void alpha(u32 dstX, u32 dstY, char c);
	void copy(u32 dstX, u32 dstY, char c);

	glyphData_t glyph[256];

	RSX* _rsx;
	u32 _color;
};

#endif /* FONT_H_ */
