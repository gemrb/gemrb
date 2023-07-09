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

#include "FogRenderer.h"

#include "AnimationFactory.h"
#include "GameData.h"
#include "Video/Video.h"

namespace GemRB {

constexpr EnumArray<FogRenderer::Direction, BlitFlags> FogRenderer::BAM_FLAGS;

EnumArray<FogRenderer::Direction, Holder<Sprite2D>> FogRenderer::LoadFogSprites()
{
	auto anim = gamedata->GetFactoryResourceAs<const AnimationFactory>("fogowar", IE_BAM_CLASS_ID);
	if (!anim) {
		return {};
	}
	
	EnumArray<FogRenderer::Direction, Holder<Sprite2D>> sprites;

	sprites[Direction::N] = anim->GetFrame(0, 0); // horizontal edge
	sprites[Direction::W] = anim->GetFrame(1, 0); // vertical edge
	sprites[Direction::NW] = anim->GetFrame(2, 0); // corner
	sprites[Direction::S] = sprites[Direction::N];
	sprites[Direction::SW] = sprites[Direction::NW];
	sprites[Direction::E] = sprites[Direction::W];
	sprites[Direction::NE] = sprites[Direction::NW];
	sprites[Direction::SE] = sprites[Direction::NE];
	
	return sprites;
}

FogRenderer::FogRenderer(bool doBAMRendering) :
	videoCanRenderGeometry(!doBAMRendering && VideoDriver->CanDrawRawGeometry()),
	fogVertices(24),
	fogColors(12)
{
	fogSprites = LoadFogSprites();
}

void FogRenderer::DrawFog(const FogMapData& mapData) {
	const Size& fogSize = mapData.fogSize;
	auto largeFog = mapData.largeFog;

	// Unbind this data from the object when there is more than one public method
	this->vp = mapData.vp;
	this->mapSize = mapData.mapSize;
	this->start = Clamp(ConvertPointToFog(vp.origin), Point(), Point(fogSize.w, fogSize.h));
	this->end = Clamp(ConvertPointToFog(vp.Maximum()) + Point(2 + largeFog, 2 + largeFog), Point(), Point(fogSize.w, fogSize.h));

	this->p0 = {
		(start.x * CELL_SIZE - vp.x) - (largeFog * CELL_SIZE / 2),
		(start.y * CELL_SIZE - vp.y) - (largeFog * CELL_SIZE / 2)
	};

	DrawVPBorders();

	for (int y = start.y; y < end.y; y++) {
		int unexploredQueue = 0;
		int shroudedQueue = 0;
		int x = start.x;
		for (; x < end.x; x++) {
			Point cell{x, y};

			if (IsUncovered(cell, mapData.exploredMask)) {
				if (unexploredQueue) {
					FillFog(ConvertPointToScreen(cell.x - unexploredQueue, cell.y), unexploredQueue, OPAQUE_FOG);
					unexploredQueue = 0;
				}

				if (IsUncovered(cell, mapData.visibleMask)) {
					if (shroudedQueue) {
						FillFog(ConvertPointToScreen(cell.x - shroudedQueue, cell.y), shroudedQueue, TRANSPARENT_FOG);
						shroudedQueue = 0;
					}
					DrawVisibleCell(cell, mapData.visibleMask);
				} else {
					// coalesce all horizontally adjacent shrouded cells
					++shroudedQueue;
				}

				DrawExploredCell(cell, mapData.exploredMask);
			} else {
				// coalesce all horizontally adjacent unexplored cells
				++unexploredQueue;
				if (shroudedQueue) {
					FillFog(ConvertPointToScreen(cell.x - shroudedQueue, cell.y), shroudedQueue, TRANSPARENT_FOG);
					shroudedQueue = 0;
				}
			}
		}

		if (shroudedQueue) {
			FillFog(ConvertPointToScreen(x - (shroudedQueue + unexploredQueue), y), shroudedQueue, TRANSPARENT_FOG);
		}

		if (unexploredQueue) {
			FillFog(ConvertPointToScreen(x - unexploredQueue, y), unexploredQueue, OPAQUE_FOG);
		}
	}
}

Point FogRenderer::ConvertPointToFog(Point p) {
	return Point(p.x / 32, p.y / 32);
}

Point FogRenderer::ConvertPointToScreen(int x, int y) const {
	x = (x - start.x) * CELL_SIZE + p0.x;
	y = (y - start.y) * CELL_SIZE + p0.y;

	return Point(x, y);
}

void FogRenderer::DrawExploredCell(Point p, const Bitmap *exploredMask) {
	auto IsExplored = [=](int x, int y) {
		return IsUncovered({x, y}, exploredMask);
	};
	Point sp = ConvertPointToScreen(p.x, p.y);

	Direction dirs = IsExplored(p.x, p.y - 1) ? Direction::O : Direction::N;
	if (!IsExplored(p.x - 1, p.y)) dirs |= Direction::W;
	if (!IsExplored(p.x, p.y + 1)) dirs |= Direction::S;
	if (!IsExplored(p.x + 1, p.y)) dirs |= Direction::E;

	if (dirs != Direction::O && !DrawFogCellByDirection(sp, dirs, BlitFlags::BLENDED)) {
		FillFog(sp, 1, OPAQUE_FOG);
	}

	if (videoCanRenderGeometry) {
		dirs = Direction::O;

		if (!IsExplored(p.x - 1, p.y - 1)) dirs |= Direction::NW;
		if (!IsExplored(p.x + 1, p.y - 1)) dirs |= Direction::NE;
		if (dirs != Direction::O) {
			DrawFogSmoothing(sp, dirs, OPAQUE_FOG, Direction::O);
		}

		dirs = Direction::O;
		if (!IsExplored(p.x - 1, p.y + 1)) dirs |= Direction::SW;
		if (!IsExplored(p.x + 1, p.y + 1)) dirs |= Direction::SE;
		if (dirs != Direction::O) {
			DrawFogSmoothing(sp, dirs, OPAQUE_FOG, Direction::O);
		}
	}
}

void FogRenderer::DrawVisibleCell(Point p, const Bitmap *visibleMask) {
	auto IsVisible = [=](int x, int y) {
		return IsUncovered({x, y}, visibleMask);
	};
	Point sp = ConvertPointToScreen(p.x, p.y);

	Direction dirs = IsVisible(p.x, p.y - 1) ? Direction::O : Direction::N;
	if (!IsVisible(p.x - 1, p.y)) dirs |= Direction::W;
	if (!IsVisible(p.x, p.y + 1)) dirs |= Direction::S;
	if (!IsVisible(p.x + 1, p.y)) dirs |= Direction::E;

	if (dirs != Direction::O && !DrawFogCellByDirection(sp, dirs, TRANSPARENT_FOG)) {
		FillFog(sp, 1, TRANSPARENT_FOG);
	}

	if (videoCanRenderGeometry) {
		Direction smoothDirs = Direction::O;

		if (!IsVisible(p.x - 1, p.y - 1)) smoothDirs |= Direction::NW;
		if (!IsVisible(p.x + 1, p.y - 1)) smoothDirs |= Direction::NE;
		if (smoothDirs != Direction::O) {
			DrawFogSmoothing(sp, smoothDirs, TRANSPARENT_FOG, dirs);
		}

		smoothDirs = Direction::O;
		if (!IsVisible(p.x - 1, p.y + 1)) smoothDirs |= Direction::SW;
		if (!IsVisible(p.x + 1, p.y + 1)) smoothDirs |= Direction::SE;
		if (smoothDirs != Direction::O) {
			DrawFogSmoothing(sp, smoothDirs, TRANSPARENT_FOG, dirs);
		}
	}
}

void FogRenderer::DrawFogCellBAM(Point p, Direction direction, BlitFlags flags) {
	VideoDriver->BlitGameSprite(fogSprites[direction], p, flags | BAM_FLAGS[direction]);
}

void FogRenderer::DrawFogCellVertices(Point p, Direction direction, BlitFlags flags) {
	SetFogVerticesByOrigin(p);

	// don't always make center vertices dark, see below
	uint16_t halvingBits = 1 | (1 << 3) | (1 << 6) | (1 << 9);
	uint16_t fillBits = halvingBits;

	if ((direction & Direction::N) != Direction::O) {
		fillBits |= (3 << 1) | (1 << 4) | (1 << 11);
	}
	if ((direction & Direction::S) != Direction::O) {
		fillBits |= (3 << 7) | (1 << 5) | (1 << 10);
	}
	if ((direction & Direction::E) != Direction::O) {
		fillBits |= (3 << 4) | (1 << 2) | (1 << 7);
	}
	if ((direction & Direction::W) != Direction::O) {
		fillBits |= (3 << 10) | (1 << 1) | (1 << 8);
	}

	Color baseColor{0, 0, 0, 255};
	if (flags & BlitFlags::HALFTRANS) {
		baseColor = {0, 0, 0, 128};
	}

	for (size_t i = 0; i < fogColors.size(); ++i) {
		fogColors[i] = baseColor;

		if ((fillBits & (1 << i)) == 0) {
			// 0xDB6 means that all outer vertices are requested, i. e. fill
			// otherwise take down the alpha a little as it would be too dark
			if ((halvingBits & (1 << i)) && fillBits != 0xDB6) {
				fogColors[i].a = baseColor.a / 2;
			} else {
				fogColors[i].a = 0;
			}
		}
	}

	VideoDriver->DrawRawGeometry(fogVertices, fogColors, BlitFlags::BLENDED);
}

void FogRenderer::DrawFogSmoothing(Point p, Direction direction, BlitFlags flags, Direction adjacentDir) {
	// We need this when relying on smooth, alpha interpolation. The FOGOWAR.bam patterns
	// cannot be simply replaced by one triangle fan: Given how the interpolation works, there
	// will be ugly eges since usually the darkening is notable for way too long. We could also
	// transform the vertices into something more fitting but that'd probably require more code.
	//
	// Instead, look for cells that have an over-the-edge fog cell and do a little smoothing
	// to hide the edges.

	SetFogVerticesByOrigin(p);

	uint16_t fillBits = 0;

	// We use adjacentDir to check whether the corners have already been used in the last
	// draw pass of the edge-adjacent fog. This prevents over-blending (darkening) in the case
	// that something has one edge and one corner-adjacent cover.
	if ((direction & Direction::NW) == Direction::NW && !(adjacentDir & Direction::NW)) {
		fillBits |= (1 << 1) | (1 << 11);
	}
	if ((direction & Direction::NE) == Direction::NE && !(adjacentDir & Direction::NE)) {
		fillBits |= (1 << 2) | (1 << 4);
	}
	if ((direction & Direction::SE) == Direction::SE && !(adjacentDir & Direction::SE)) {
		fillBits |= (1 << 5) | (1 << 7);
	}
	if ((direction & Direction::SW) == Direction::SW && !(adjacentDir & Direction::SW)) {
		fillBits |= (1 << 8) | (1 << 10);
	}

	Color baseColor{0, 0, 0, 255};
	if (flags & BlitFlags::HALFTRANS) {
		baseColor = {0, 0, 0, 128};
	}

	for (size_t i = 0; i < fogColors.size(); ++i) {
		fogColors[i] = baseColor;

		if ((fillBits & (1 << i)) == 0) {
			fogColors[i].a = 0;
		}
	}

	VideoDriver->DrawRawGeometry(fogVertices, fogColors, BlitFlags::BLENDED);
}

bool FogRenderer::DrawFogCellByDirection(Point p, Direction direction, BlitFlags flags) {
	if (videoCanRenderGeometry) {
		DrawFogCellVertices(p, direction, flags);
		// accepts any adjacent-direction setup in one call
		return true;
	} else {
		return DrawFogCellByDirectionBAMs(p, direction, flags);
	}
}

bool FogRenderer::DrawFogCellByDirectionBAMs(Point p, Direction direction, BlitFlags flags) {
	// supress compiler warnings about case not in enum by using UnderType
	switch (UnderType(direction) & 0xF) {
		case UnderType(Direction::N):
		case UnderType(Direction::W):
		case UnderType(Direction::NW):
		case UnderType(Direction::S):
		case UnderType(Direction::SW):
		case UnderType(Direction::E):
		case UnderType(Direction::NE):
		case UnderType(Direction::SE):
			DrawFogCellBAM(p, direction, flags);
			return true;
		case UnderType(Direction::N|Direction::S):
			DrawFogCellBAM(p, Direction::N, flags);
			DrawFogCellBAM(p, Direction::S, flags);
			return true;
		case UnderType(Direction::NW|Direction::SW):
			DrawFogCellBAM(p, Direction::NW, flags);
			DrawFogCellBAM(p, Direction::SW, flags);
			return true;
		case UnderType(Direction::W|Direction::E):
			DrawFogCellBAM(p, Direction::W, flags);
			DrawFogCellBAM(p, Direction::E, flags);
			return true;
		case UnderType(Direction::NW|Direction::NE):
			DrawFogCellBAM(p, Direction::NW, flags);
			DrawFogCellBAM(p, Direction::NE, flags);
			return true;
		case UnderType(Direction::NE|Direction::SE):
			DrawFogCellBAM(p, Direction::NE, flags);
			DrawFogCellBAM(p, Direction::SE, flags);
			return true;
		case UnderType(Direction::SW|Direction::SE):
			DrawFogCellBAM(p, Direction::SW, flags);
			DrawFogCellBAM(p, Direction::SE, flags);
			return true;
		default: // a fully surrounded tile is filled
			return false;
	}
}

void FogRenderer::DrawVPBorder(Point p, Direction direction, const Region& r, BlitFlags flags) {
	if (videoCanRenderGeometry) {
		DrawFogCellVertices(p, direction, OPAQUE_FOG);
	} else {
		VideoDriver->BlitSprite(fogSprites[direction], p, &r, flags);
	}
}

void FogRenderer::DrawVPBorders() {
	// the amount of fuzzing to apply to map edges when the viewport overscans
	constexpr int FUZZ_AMT = 8;

	if (vp.y < 0) { // north border
		Region r(0, 0, vp.w, -vp.y);
		VideoDriver->DrawRect(r, ColorBlack, true);
		r.y += r.h;
		r.h = FUZZ_AMT;
		for (int x = r.x + p0.x; x < r.w; x += CELL_SIZE) {
			DrawVPBorder(Point(x, r.y), Direction::N, r, BAM_FLAGS[Direction::N]);
		}
	}

	if (vp.y + vp.h > mapSize.h) { // south border
		Region r(0, mapSize.h - vp.y, vp.w, vp.y + vp.h - mapSize.h);
		VideoDriver->DrawRect(r, ColorBlack, true);
		r.y -= FUZZ_AMT;
		r.h = FUZZ_AMT;
		for (int x = r.x + p0.x; x < r.w; x += CELL_SIZE) {
			DrawVPBorder(Point(x, r.y), Direction::S, r, BAM_FLAGS[Direction::S]);
		}
	}

	if (vp.x < 0) { // west border
		Region r(0, std::max(0, -vp.y), -vp.x, mapSize.h);
		VideoDriver->DrawRect(r, ColorBlack, true);
		r.x += r.w;
		r.w = FUZZ_AMT;
		for (int y = r.y + p0.y; y < r.h; y += CELL_SIZE) {
			DrawVPBorder(Point(r.x, y), Direction::W, r, BAM_FLAGS[Direction::W]);
		}
	}

	if (vp.x + vp.w > mapSize.w) { // east border
		Region r(mapSize.w -vp.x, std::max(0, -vp.y), vp.x + vp.w - mapSize.w, mapSize.h);
		VideoDriver->DrawRect(r, ColorBlack, true);
		r.x -= FUZZ_AMT;
		r.w = FUZZ_AMT;
		for (int y = r.y + p0.y; y < r.h; y += CELL_SIZE) {
			DrawVPBorder(Point(r.x, y), Direction::E, r, BAM_FLAGS[Direction::E]);
		}
	}
}

void FogRenderer::FillFog(Point p, int numRowItems, BlitFlags flags) {
	Region r(p, Size(CELL_SIZE * numRowItems, CELL_SIZE));
	VideoDriver->DrawRect(r, ColorBlack, true, flags);
}

bool FogRenderer::IsUncovered(Point p, const Bitmap *mask) {
	if (mask == nullptr) return true;

	return mask->GetAt(p, false);
}

void FogRenderer::SetFogVerticesByOrigin(Point p) {
	// Triangle fan of 4: center
	fogVertices[0] = fogVertices[6] = fogVertices[12] = fogVertices[18] = p.x + CELL_SIZE / 2;
	fogVertices[1] = fogVertices[7] = fogVertices[13] = fogVertices[19] = p.y + CELL_SIZE / 2;
	// Top (l, r)
	fogVertices[2] = p.x;
	fogVertices[3] = p.y;
	fogVertices[4] = p.x + CELL_SIZE;
	fogVertices[5] = p.y;
	// Right (t, b)
	fogVertices[8] = p.x + CELL_SIZE;
	fogVertices[9] = p.y;
	fogVertices[10] = p.x + CELL_SIZE;
	fogVertices[11] = p.y + CELL_SIZE;
	// Bottom (r, l)
	fogVertices[14] = p.x + CELL_SIZE;
	fogVertices[15] = p.y + CELL_SIZE;
	fogVertices[16] = p.x;
	fogVertices[17] = p.y + CELL_SIZE;
	// left (b, t)
	fogVertices[20] = p.x;
	fogVertices[21] = p.y + CELL_SIZE;
	fogVertices[22] = p.x;
	fogVertices[23] = p.y;
}

}
