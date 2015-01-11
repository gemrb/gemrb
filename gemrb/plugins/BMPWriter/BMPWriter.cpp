#include "BMPWriter.h"

#include "Interface.h"
#include "Video.h"

#include <cstring>

using namespace GemRB;

#define BMP_HEADER_SIZE  54 //FIXME: duplicate

#define GET_SCANLINE_LENGTH(width, bitsperpixel)  (((width)*(bitsperpixel)+7)/8)

BMPWriter::BMPWriter()
{
}

BMPWriter::~BMPWriter()
{
}

void BMPWriter::PutImage(DataStream *output, Sprite2D *spr)
{
	ieDword tmpDword;
	ieWord tmpWord;

	// FIXME
	ieDword Width = spr->Width;
	ieDword Height = spr->Height;
	char filling[3] = {'B','M'};
	ieDword PaddedRowLength = GET_SCANLINE_LENGTH(Width,24);
	int stuff = (4-(PaddedRowLength&3))&3; // rounding it up to 4 bytes boundary
	PaddedRowLength+=stuff;
	ieDword fullsize = PaddedRowLength*Height;

	//always save in truecolor (24 bit), no palette
	output->Write( filling, 2);
	tmpDword = fullsize+BMP_HEADER_SIZE;  // FileSize
	output->WriteDword( &tmpDword);
	tmpDword = 0;
	output->WriteDword( &tmpDword);       // ??
	tmpDword = BMP_HEADER_SIZE;           // DataOffset
	output->WriteDword( &tmpDword);
	tmpDword = 40;                        // Size
	output->WriteDword( &tmpDword);
	output->WriteDword( &Width);
	output->WriteDword( &Height);
	tmpWord = 1;                          // Planes
	output->WriteWord( &tmpWord);
	tmpWord = 24; //24 bits               // BitCount
	output->WriteWord( &tmpWord);
	tmpDword = 0;                         // Compression
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);       // ImageSize
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);

	memset( filling,0,sizeof(filling) );
	for (unsigned int y=0;y<Height;y++) {
		for (unsigned int x=0;x<Width;x++) {
			Color c = spr->GetPixel(x,Height-y-1);

			output->Write( &c.b, 1);
			output->Write( &c.g, 1);
			output->Write( &c.r, 1);
		}
		output->Write( filling, stuff);
	}

	Sprite2D::FreeSprite(spr);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD48051E, "BMP File Writer")
PLUGIN_CLASS(PLUGIN_IMAGE_WRITER_BMP, BMPWriter)
END_PLUGIN()
