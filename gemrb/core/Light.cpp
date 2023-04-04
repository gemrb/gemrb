/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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

#include "Light.h"

#include "Geometry.h"
#include "Interface.h"
#include "Video/Video.h"

namespace GemRB {

static PaletteHolder GetLightPalette() noexcept
{
	static struct Init {
		PaletteHolder pal;
		Init() noexcept {
			Color cols[256] {ColorBlack};
			for (int i = 1; i < 256; ++i) {
				cols[i] = ColorWhite;
				cols[i].a = i;
			}
			pal = MakeHolder<Palette>(std::begin(cols), std::end(cols));
		}
	} data;

	return data.pal;
}

Holder<Sprite2D> CreateLight(const Size& size, uint8_t intensity) noexcept
{
	uint8_t* px = (uint8_t*)calloc(size.w, size.h);
	
	const Point& radii = size.Center();
	const float maxr = std::max(radii.x, radii.y);
	Region r(Point() - radii, size);
	
	const auto points = PlotEllipse(r);
	for (size_t i = 0; i < points.size(); i += 4) {
		const Point& q1 = points[i];
		const Point& q2 = points[i + 1];
		assert(q1.y == q2.y);
		
		const Point& q3 = points[i + 2];
		const Point& q4 = points[i + 3];
		assert(q3.y == q4.y);
		
		for (int x = q1.x; x >= 0; --x) {
			// by the power of Pythagurus
			int hyp = std::hypot(x, q1.y);
			int dist = (hyp / maxr) * intensity;
			assert(dist >= 0 && dist <= intensity);
			uint8_t light = Clamp<uint8_t>(intensity - dist, 0, intensity);
			px[(radii.y + q1.y) * size.w + radii.x + x] = light; // quad 1
			px[(radii.y + q2.y) * size.w + radii.x - x] = light; // quad 2
			px[(radii.y + q3.y) * size.w + radii.x - x] = light; // quad 3
			px[(radii.y + q4.y) * size.w + radii.x + x] = light; // quad 4
		}
	}

	r.origin = radii;
	PixelFormat fmt = PixelFormat::Paletted8Bit(GetLightPalette(), true, 0);
	return core->GetVideoDriver()->CreateSprite(r, px, fmt);
}

}
