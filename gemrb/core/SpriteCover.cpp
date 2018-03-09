/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "SpriteCover.h"

#include "Interface.h"
#include "Video.h"

namespace GemRB {

SpriteCover::SpriteCover(const Point& wp, const Region& rgn, int dither)
	: wp(wp)
{
	this->dither = dither;
	mask = core->GetVideoDriver()->CreateSprite8(rgn.w, rgn.h, NULL, NULL);
	mask->XPos = rgn.x;
	mask->YPos = rgn.y;
}

SpriteCover::~SpriteCover()
{
	mask->release();
}

bool SpriteCover::Covers(int x, int y, int xpos, int ypos,
						 int width, int height) const
{
	// if basepoint changed, no longer valid
	if (x != wp.x || y != wp.y) return false;

	// top-left not covered
	if (xpos > mask->XPos || ypos > mask->YPos) return false;

	// bottom-right not covered
	if (width-xpos > mask->Width - mask->XPos || height - ypos > mask->Height - mask->YPos) return false;

	return true;
}

// flags: 0 - never dither (full cover)
//	1 - dither if polygon wants it
//	2 - always dither
void SpriteCover::AddPolygon(Wall_Polygon* poly)
{
	// possible TODO: change the cover to use a set of intervals per line?
	// advantages: faster
	// disadvantages: makes the blitter much more complex

	int xoff = wp.x - mask->XPos;
	int yoff = wp.y - mask->YPos;

	unsigned char* srcdata = static_cast<unsigned char*>(mask->LockSprite());

	std::list<Trapezoid>::iterator iter;
	for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end();
		 ++iter)
	{
		int y_top = iter->y1 - yoff; // inclusive
		int y_bot = iter->y2 - yoff; // exclusive

		if (y_top < 0) y_top = 0;
		if ( y_bot > mask->Height) y_bot = mask->Height;
		if (y_top >= y_bot) continue; // clipped

		int ledge = iter->left_edge;
		int redge = iter->right_edge;
		Point& a = poly->points[ledge];
		Point& b = poly->points[(ledge+1)%(poly->count)];
		Point& c = poly->points[redge];
		Point& d = poly->points[(redge+1)%(poly->count)];

		unsigned char* line = srcdata + (y_top)*mask->Width;
		for (int sy = y_top; sy < y_bot; ++sy) {
			int py = sy + yoff;

			// TODO: maybe use a 'real' line drawing algorithm to
			// compute these values faster.

			int lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
			int rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y) + 1;

			lt -= xoff;
			rt -= xoff;

			if (lt < 0) lt = 0;
			if (rt > mask->Width) rt = mask->Width;
			if (lt >= rt) { line += mask->Width; continue; } // clipped
			bool doDither;

			if (dither == 1) {
				doDither = poly->wall_flag & WF_DITHER;
			} else {
				doDither = dither;
			}

			memset (line+lt, (doDither) ? 0x80 : 0xff, rt-lt);
			line += mask->Width;
		}
	}

	mask->UnlockSprite();
}

}
