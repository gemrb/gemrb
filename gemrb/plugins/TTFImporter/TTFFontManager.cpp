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

#include <math.h>

#include "Interface.h"
#include "Palette.h"
#include "TTFFont.h"
#include "TTFFontManager.h"

using namespace GemRB;

FT_Library library = NULL;

void loadFT()
{
	FT_Error error = FT_Init_FreeType( &library );
	if ( error ) {
		LogFTError(error);
	}
}

void destroyFT()
{
	if (library) {
		FT_Done_FreeType( library );
	}
}

unsigned long TTFFontManager::read(FT_Stream		  stream,
								   unsigned long   offset,
								   unsigned char*  buffer,
								   unsigned long   count )
{
	DataStream* dstream = (DataStream*)stream->descriptor.pointer;
	dstream->Seek(offset, GEM_STREAM_START);
	return dstream->Read(buffer, count);
}

TTFFontManager::~TTFFontManager(void)
{
	Close();
}

TTFFontManager::TTFFontManager(void)
: ftStream(NULL), face(NULL)
{}

bool TTFFontManager::Open(DataStream* stream)
{
	Close();
	if (stream) {
		FT_Error error;

		ftStream = (FT_Stream)calloc(sizeof(*ftStream), 1);
		ftStream->read = read;
		ftStream->descriptor.pointer = stream;
		ftStream->pos = stream->GetPos();
		ftStream->size = stream->Size();

		FT_Open_Args args = FT_Open_Args();
		args.flags = FT_OPEN_STREAM;
		args.stream = ftStream;

		error = FT_Open_Face( library, &args, 0, &face );
		if( error ) {
			LogFTError(error);
			Close();
			return false;
		}

		// we always convert to UTF-16
		// TODO: maybe we should allow an override encoding?
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		return true;
	}
	return false;
}

void TTFFontManager::Close()
{
	if (face) {
		FT_Done_Face(face);
	}
	if (ftStream) {
		free(ftStream);
		ftStream = NULL;
	}
}

Font* TTFFontManager::GetFont(unsigned short ptSize,
							  FontStyle style, Palette* pal)
{
	if (!pal) {
		Color fore = {0xFF, 0xFF, 0xFF, 0}; //white
		Color back = {0x00, 0x00, 0x00, 0}; //black
		pal = core->CreatePalette( fore, back );
		pal->CreateShadedAlphaChannel();
	}
	return new TTFFont(face, ptSize, style, pal);
}

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427C, "TTF Font Importer")
PLUGIN_RESOURCE(TTFFontManager, "ttf")
PLUGIN_INITIALIZER(loadFT)
PLUGIN_CLEANUP(destroyFT)
END_PLUGIN()
