#include "BMPWriter.h"

using namespace GemRB;

#define BMP_HEADER_SIZE 54 //FIXME: duplicate

#define GET_SCANLINE_LENGTH(width, bitsperpixel) (((width) * (bitsperpixel) + 7) / 8)

void BMPWriter::PutImage(DataStream* output, Holder<Sprite2D> spr)
{
	// FIXME
	ieDword Width = spr->Frame.w;
	ieDword Height = spr->Frame.h;
	char filling[3] = { 'B', 'M' };
	ieDword PaddedRowLength = GET_SCANLINE_LENGTH(Width, 24);
	int stuff = (4 - (PaddedRowLength & 3)) & 3; // rounding it up to 4 bytes boundary
	PaddedRowLength += stuff;
	ieDword fullsize = PaddedRowLength * Height;

	//always save in truecolor (24 bit), no palette
	output->Write(filling, 2);
	output->WriteDword(fullsize + BMP_HEADER_SIZE); // FileSize
	output->WriteDword(0); // ??
	output->WriteDword(BMP_HEADER_SIZE); // DataOffset
	output->WriteDword(40); // Size
	output->WriteDword(Width);
	output->WriteDword(Height);
	output->WriteWord(1); // Planes
	output->WriteWord(24); // BitCount
	output->WriteFilling(24); // Compression

	auto it = spr->GetIterator(IPixelIterator::Direction::Forward, IPixelIterator::Direction::Reverse);
	for (unsigned int y = 0; y < Height; y++) {
		for (unsigned int x = 0; x < Width; ++x, ++it) {
			const Color& c = it.ReadRGBA();

			output->Write(&c.b, 1);
			output->Write(&c.g, 1);
			output->Write(&c.r, 1);
		}
		output->WriteFilling(stuff);
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD48051E, "BMP File Writer")
PLUGIN_CLASS(PLUGIN_IMAGE_WRITER_BMP, BMPWriter)
END_PLUGIN()
