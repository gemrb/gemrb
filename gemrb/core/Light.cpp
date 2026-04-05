// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Light.h"

#include "Geometry.h"

#include "Video/Video.h"

namespace GemRB {

static Holder<Palette> GetLightPalette() noexcept
{
	static struct Init {
		Holder<Palette> pal;
		Init() noexcept
		{
			Color cols[256] { ColorBlack };
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
	uint8_t* px = (uint8_t*) calloc(size.w, size.h);

	const Point& radii = size.Center();
	const float maxr = std::max<float>(radii.x, radii.y);
	Region r(Point() - radii, size);

	const auto points = PlotEllipse(r);
	for (size_t i = 0; i < points.size(); i += 4) {
		const BasePoint& q1 = points[i];
		const BasePoint& q2 = points[i + 1];
		assert(q1.y == q2.y);

		const BasePoint& q3 = points[i + 2];
		const BasePoint& q4 = points[i + 3];
		assert(q3.y == q4.y);

		for (int x = q1.x; x >= 0; --x) {
			// by the power of Pythagoras
			uint8_t hyp = std::hypot<uint8_t>(x, q1.y);
			uint8_t dist = static_cast<uint8_t>((hyp / maxr) * intensity);
			assert(dist <= intensity);
			uint8_t light = Clamp<uint8_t>(intensity - dist, 0, intensity);
			px[(radii.y + q1.y) * size.w + radii.x + x] = light; // quad 1
			px[(radii.y + q2.y) * size.w + radii.x - x] = light; // quad 2
			px[(radii.y + q3.y) * size.w + radii.x - x] = light; // quad 3
			px[(radii.y + q4.y) * size.w + radii.x + x] = light; // quad 4
		}
	}

	r.origin = radii;
	PixelFormat fmt = PixelFormat::Paletted8Bit(GetLightPalette(), true, 0);
	return VideoDriver->CreateSprite(r, px, fmt);
}

}
