/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "PVRZImporter.h"

#include "RGBAColor.h"

#include "Logging/Logging.h"
#include "Video/Video.h"

using namespace GemRB;

bool PVRZImporter::Import(DataStream* str)
{
	ieDword signature;
	bool decompressed = false;

	while (true) {
		str->ReadDword(signature);

		if (signature == 0x50565203) {
			str->SetBigEndianness(true);
		} else if (signature != 0x03525650) {
			if (!decompressed) {
				str = DecompressStream(str);
				decompressed = true;
				continue;
			} else {
				Log(ERROR, "PVRZIpporter", "Unsupported file format");
				return false;
			}
		}
		break;
	}

	// ignoring flags
	str->Seek(4, GEM_CURRENT_POS);

	uint64_t rawFormat = 0;
	str->ReadScalar<uint64_t>(rawFormat);

	// if upper 4B are non-zero, it's some customization; ignored
	if ((rawFormat & (0xFFFFFFFFULL << 32)) == 0) {
		switch (rawFormat & 0xFFFFFFFF) {
			case 7:
				format = PVRZFormat::DXT1;
				break;
			case 11:
				format = PVRZFormat::DXT5;
				break;
			default:
				format = PVRZFormat::UNSUPPORTED;
		}
	}

	if (format == PVRZFormat::UNSUPPORTED) {
		Log(ERROR, "PVRZImporter", "Unsupported texture format");
		return false;
	}

	// ignoring the color space
	str->Seek(4, GEM_CURRENT_POS);

	// 0: normalized unsigned byte
	ieDword channelType = 0;
	str->ReadDword(channelType);
	if (channelType != 0) {
		Log(ERROR, "PVRZImporter", "Unsupported channel access type");
		return false;
	}

	str->ReadScalar<int, ieDword>(size.h);
	str->ReadScalar<int, ieDword>(size.w);
	if (size.h < 0 || size.w < 0) {
		Log(ERROR, "PVRZImporter", "Negative or overflown rectangular dimension");
	}

	ieDword tmp = 0;
	for (uint8_t i = 0; i < 4; ++i) {
		str->ReadDword(tmp);
		if (tmp != 1) {
			Log(ERROR, "PVRZImporter", "depth, #faces, #surfaces or #mipmaps != 1, unsupported.");
			return false;
		}
	}

	ieDword metaDataSize = 0;
	str->ReadDword(metaDataSize);
	// there is currently nothing in there for us, or nothing that we know we need
	if (metaDataSize > 0) {
		str->Seek(metaDataSize, GEM_CURRENT_POS);
	}

	auto dataSize = str->Remains();
	data.resize(dataSize);
	str->Read(reinterpret_cast<void*>(data.data()), dataSize);

	return true;
}

Holder<Sprite2D> PVRZImporter::GetSprite2D()
{
	return GetSprite2D(Region(0, 0, size.w, size.h));
}

Holder<Sprite2D> PVRZImporter::GetSprite2D(Region&& region)
{
	if (region.x < 0 || (region.x + region.w) > size.w || region.y < 0 || (region.y + region.h) > size.h) {
		Log(ERROR, "PVRZImporter", "Out-of-bounds access");
		return {};
	}

	if (region.w == 0 || region.h == 0) {
		return {};
	}

	switch (format) {
		case PVRZFormat::DXT1:
			return getSprite2DDXT1(std::move(region));
		case PVRZFormat::DXT5:
			return getSprite2DDXT5(std::move(region));
		default:
			return {};
	}
}

std::tuple<uint16_t, uint16_t> PVRZImporter::extractPalette(size_t offset, std::array<uint8_t, 6>& colors) const
{
	uint16_t color1 = *reinterpret_cast<const uint16_t*>(&data[offset]);
	uint16_t color2 = *reinterpret_cast<const uint16_t*>(&data[offset + 2]);

	auto convert = [&](uint16_t color, uint8_t outOffset) {
		colors[outOffset] = ((color << 3) & 0xF8) | ((color >> 2) & 0x7);
		colors[outOffset + 1] = ((color >> 3) & 0xFC) | ((color >> 9) & 0x3);
		colors[outOffset + 2] = ((color >> 8) & 0xF8) | ((color >> 13) & 0x7);
	};

	convert(color1, 0);
	convert(color2, 3);

	return { color1, color2 };
}

uint16_t PVRZImporter::GetBlockPixelMask(const Region& region, const Region& grid, int x, int y)
{
	// 16bit to represent 4 rows x 4 cols as bitmap,
	// unset pixels will be skipped, so if 1-3 pixels lap into a block,
	// there is something like:
	//
	// 1 1 0 0
	// 1 1 0 0
	// 0 0 0 0
	// 0 0 0 0
	//
	// The LSB is (0, 0) -- as in the DXT blocks.
	uint16_t pixelMask = 0xFFFF;

	// Top margin
	if (y == grid.y) {
		int yOverlap = region.y % 4;
		if (yOverlap != 0) {
			for (int by = 0; by < yOverlap; ++by) {
				pixelMask &= ~(0xF << (4 * by));
			}
		}
	}

	// Bottom margin
	if (y == grid.h - 1) {
		int yOverlap = (region.y + region.h) % 4;
		if (yOverlap != 0) {
			for (int by = 4; by > yOverlap; --by) {
				pixelMask &= ~(0xF << (4 * (by - 1)));
			}
		}
	}

	// Left margin
	if (x == grid.x) {
		int xOverlap = region.x % 4;
		if (xOverlap != 0) {
			uint8_t rowMask = 0;

			for (int bx = 0; bx < xOverlap; ++bx) {
				rowMask |= (1 << bx);
			}
			for (int by = 0; by < 4; ++by) {
				pixelMask &= ~(rowMask << (4 * by));
			}
		}
	}

	// Right margin
	if (x == grid.w - 1) {
		int xOverlap = (region.x + region.w) % 4;
		if (xOverlap != 0) {
			uint8_t rowMask = 0;
			uint16_t matrix = 0;

			// Determine what a single row (4 bit) looks like
			for (int bx = 0; bx < xOverlap; ++bx) {
				rowMask |= (1 << bx);
			}
			// Rollout to following rows
			for (int by = 0; by < 4; ++by) {
				matrix <<= 4;
				matrix |= rowMask;
			}
			// Cross with rows that are still there
			pixelMask &= matrix;
		}
	}

	return pixelMask;
}

Holder<Sprite2D> PVRZImporter::getSprite2DDXT1(Region&& region) const
{
	PixelFormat fmt = PixelFormat::ARGB32Bit();
	uint32_t* uncompressedData = reinterpret_cast<uint32_t*>(malloc(region.size.Area() * 4));
	std::fill(uncompressedData, uncompressedData + region.size.Area(), 0);

	std::array<uint8_t, 6> colors;
	Point blockOrigin { region.x % 4, region.y % 4 };

	Region grid { region.x / 4, region.y / 4, (region.x + region.w) / 4, (region.y + region.h) / 4 };
	if ((region.x + region.w) % 4 != 0) {
		grid.w += 1;
	}
	if ((region.y + region.h) % 4 != 0) {
		grid.h += 1;
	}

	for (int y = grid.y; y < grid.h; ++y) {
		for (int x = grid.x; x < grid.w; ++x) {
			size_t srcDataOffset = (y * (size.w / 4) + x) * 8; // Block size is 64bit

			auto color1n2 = extractPalette(srcDataOffset, colors);
			bool col1IsGreater = std::get<0>(color1n2) > std::get<1>(color1n2);

			auto pixelMask = GetBlockPixelMask(region, grid, x, y);
			uint32_t blockValue = *(reinterpret_cast<const uint32_t*>(&data[srcDataOffset + 4])); // 4x4x2 bit
			for (uint8_t i = 0; i < 16; ++i) {
				if ((pixelMask & (1 << i)) == 0) {
					continue;
				}

				uint32_t fullColor = 0xFF000000;
				uint8_t value = (blockValue & (3 << (i * 2))) >> i * 2;

				if (value == 0) {
					fullColor |= (colors[2] << 16) | (colors[1] << 8) | colors[0];
				} else if (value == 1) {
					fullColor |= (colors[5] << 16) | (colors[4] << 8) | colors[3];
				} else if (value == 2) {
					if (col1IsGreater) {
						fullColor |= ((colors[2] * 2 + colors[5]) / 3) << 16;
						fullColor |= ((colors[1] * 2 + colors[4]) / 3) << 8;
						fullColor |= ((colors[0] * 2 + colors[3]) / 3);
					} else {
						fullColor |= ((colors[2] + colors[5]) / 2) << 16;
						fullColor |= ((colors[1] + colors[4]) / 2) << 8;
						fullColor |= ((colors[0] + colors[3]) / 2);
					}
				} else {
					if (col1IsGreater) {
						fullColor |= ((colors[2] + colors[5] * 2) / 3) << 16;
						fullColor |= ((colors[1] + colors[4] * 2) / 3) << 8;
						fullColor |= ((colors[0] + colors[3] * 2) / 3);
					} else {
						fullColor = 0;
					}
				}

				uint8_t row = i / 4;
				uint8_t column = i % 4;
				uint32_t destX = (x - grid.x) * 4 + column - blockOrigin.x;
				uint32_t destY = (y - grid.y) * 4 + row - blockOrigin.y;

				size_t destDataOffset = region.w * destY + destX;
				uncompressedData[destDataOffset] = fullColor;
			}
		}
	}

	auto spr = VideoDriver->CreateSprite(Region { 0, 0, region.w, region.h }, uncompressedData, fmt);
	return { spr };
}

Holder<Sprite2D> PVRZImporter::getSprite2DDXT5(Region&& region) const
{
	PixelFormat fmt = PixelFormat::ARGB32Bit();
	uint32_t* uncompressedData = reinterpret_cast<uint32_t*>(malloc(region.size.Area() * 4));
	std::fill(uncompressedData, uncompressedData + region.size.Area(), 0);

	std::array<uint8_t, 6> colors;
	Point blockOrigin { region.x % 4, region.y % 4 };
	Region grid { region.x / 4, region.y / 4, (region.x + region.w) / 4, (region.y + region.h) / 4 };

	if ((region.x + region.w) % 4 != 0) {
		grid.w += 1;
	}
	if ((region.y + region.h) % 4 != 0) {
		grid.h += 1;
	}

	for (int y = grid.y; y < grid.h; ++y) {
		for (int x = grid.x; x < grid.w; ++x) {
			size_t srcDataOffset = (y * (size.w / 4) + x) * 16; // Block size is 128bit

			std::array<uint8_t, 8> alpha;
			alpha[0] = data[srcDataOffset];
			alpha[1] = data[srcDataOffset + 1];

			if (alpha[0] > alpha[1]) {
				alpha[2] = (6 * alpha[0] + alpha[1]) / 7;
				alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
				alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
				alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
				alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
				alpha[7] = (alpha[0] + 6 * alpha[1]) / 7;
			} else {
				alpha[2] = (4 * alpha[0] + alpha[1]) / 5;
				alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
				alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
				alpha[5] = (alpha[0] + 4 * alpha[1]) / 5;
				alpha[6] = 0;
				alpha[7] = 255;
			}

			extractPalette(srcDataOffset + 8, colors);

			auto pixelMask = GetBlockPixelMask(region, grid, x, y);
			uint32_t blockValue = *(reinterpret_cast<const uint32_t*>(&data[srcDataOffset + 12])); // 4x4x2 bit
			uint64_t alphaBlock = *reinterpret_cast<const uint64_t*>(&data[srcDataOffset + 2]); // 6 bytes (ignoring the top 2)

			for (uint8_t i = 0; i < 16; ++i) {
				if ((pixelMask & (1 << i)) == 0) {
					continue;
				}

				uint32_t fullColor = 0;
				uint8_t value = (blockValue & (3 << (i * 2))) >> i * 2;

				if (value == 0) {
					fullColor |= (colors[2] << 16) | (colors[1] << 8) | colors[0];
				} else if (value == 1) {
					fullColor |= (colors[5] << 16) | (colors[4] << 8) | colors[3];
				} else if (value == 2) {
					fullColor |= ((colors[2] * 2 + colors[5]) / 3) << 16;
					fullColor |= ((colors[1] * 2 + colors[4]) / 3) << 8;
					fullColor |= ((colors[0] * 2 + colors[3]) / 3);
				} else {
					fullColor |= ((colors[2] + colors[5] * 2) / 3) << 16;
					fullColor |= ((colors[1] + colors[4] * 2) / 3) << 8;
					fullColor |= ((colors[0] + colors[3] * 2) / 3);
				}

				uint8_t alphaValue = alpha[(alphaBlock & (uint64_t(7) << (3 * i))) >> (3 * i)];
				fullColor |= alphaValue << 24;

				uint8_t row = i / 4;
				uint8_t column = i % 4;
				uint32_t destX = (x - grid.x) * 4 + column - blockOrigin.x;
				uint32_t destY = (y - grid.y) * 4 + row - blockOrigin.y;

				size_t destDataOffset = region.w * destY + destX;
				uncompressedData[destDataOffset] = fullColor;
			}
		}
	}

	auto spr = VideoDriver->CreateSprite(Region { 0, 0, region.w, region.h }, uncompressedData, fmt);
	return { spr };
}

int PVRZImporter::GetPalette(int, Palette&)
{
	return -1;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x813EC7, "PVRZ File Reader")
PLUGIN_IE_RESOURCE(PVRZImporter, "pvrz", (ieWord) IE_PVRZ_CLASS_ID)
END_PLUGIN()
