/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TTFFontManager.h"

#include "win32def.h"

#include "FileCache.h"
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"
#include "System/FileStream.h"

using namespace GemRB;

TTFFontManager::~TTFFontManager(void)
{
	FT_Done_FreeType( library );
}

TTFFontManager::TTFFontManager(void)
{
	FontPath[0] = 0;
	FT_Error error = FT_Init_FreeType( &library );
	if ( error ) {
		LogFTError(error);
	}
}

void TTFFontManager::LogFTError(FT_Error errCode) const
{
	static const struct
	{
		int          err_code;
		const char*  err_msg;
	} ft_errors[] = {
#include <freetype/fterrors.h>
	};
	int i;
	const char *err_msg;

	err_msg = NULL;
	for ( i=0; i < (int)((sizeof ft_errors)/(sizeof ft_errors[0])); ++i ) {
		if ( errCode == ft_errors[i].err_code ) {
			err_msg = ft_errors[i].err_msg;
			break;
		}
	}
	if ( !err_msg ) {
		err_msg = "unknown FreeType error";
	}
	Log(ERROR, "TTF Manager", err_msg);
}

bool TTFFontManager::Open(DataStream* stream)
{
	if (stream) {
		strncpy(FontPath, stream->originalfile, sizeof(FontPath));
		// we don't actually need anything from the stream.
		return true;
	}
	return false;
}

Font* TTFFontManager::GetFont(ieWord FirstChar,
							  ieWord LastChar,
							  unsigned short ptSize,
							  FontStyle style, Palette* pal)
{
	Log(MESSAGE, "TTF", "Constructing TTF font.");
	Log(MESSAGE, "TTF", "Creating font of size %i with %i characters...", ptSize, LastChar - FirstChar);

	TTF_Font* ttf = TTF_OpenFont(FontPath, ptSize);

	if (!ttf){
		Log(ERROR, "TTF", "Unable to initialize font: %s, TTFError: %s.", FontPath, TTF_GetError());
		return NULL;
	}
	if (!ptSize) {
		Log(ERROR, "TTF", "Unable to initialize font with size 0.");
		return NULL;
	}

	TTF_SetFontStyle(ttf, style);

	if (!pal) {
		Color fore = {0xFF, 0xFF, 0xFF, 0}; //white
		Color back = {0x00, 0x00, 0x00, 0}; //black
		pal = core->CreatePalette( fore, back );
		pal->CreateShadedAlphaChannel();
	}

	Sprite2D** glyphs = (Sprite2D**)malloc((LastChar - FirstChar + 1) * sizeof(Sprite2D*));

	Uint16 i; // for double byte character suport
	Uint16 chr[3];
	chr[0] = UNICODE_BOM_NATIVE;
	chr[2] = '\0';// is this needed?

	for (i = FirstChar; i <= LastChar; i++) { //printable ASCII range minus space
		chr[1] = i;

		SDL_Surface* glyph = TTF_RenderUNICODE_Shaded(ttf, chr, *(SDL_Color*)(&pal->front), *(SDL_Color*)(&pal->back));
		if (glyph){
			void* px = malloc(glyph->w * glyph->h);

			//need to convert pitch to glyph width here. video driver assumes this.
			unsigned char * dstPtr = (unsigned char*)px;
			unsigned char * srcPtr = (unsigned char*)glyph->pixels;
			for (int glyphY = 0; glyphY < glyph->h; glyphY++) {
				memcpy( dstPtr, srcPtr, glyph->w);
				srcPtr += glyph->pitch;
				dstPtr += glyph->w;
			}
			glyphs[i - FirstChar] = core->GetVideoDriver()->CreateSprite8(glyph->w, glyph->h, 8, px, pal->col, true, 0);
			glyphs[i - FirstChar]->XPos = 0;
			glyphs[i - FirstChar]->YPos = 20; //FIXME: figure out why this is required and find true value
		}
		else glyphs[i - FirstChar] = NULL;
	}
	Font* font = new Font(glyphs, FirstChar, LastChar, pal);
	pal->Release();
	font->ptSize = ptSize;
	font->style = style;
	return font;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427C, "TTF Font Importer")
PLUGIN_RESOURCE(TTFFontManager, "ttf")
END_PLUGIN()
