/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2016 The GemRB Project
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
 */

#include "ScrollView.h"

namespace GemRB {

ScrollView::ScrollView(const Region& frame)
: View(frame), contentView(Region(Point(), frame.Dimensions()))
{
	hscroll = NULL;
	vscroll = NULL;

	View::AddSubviewInFrontOfView(&contentView);
	contentView.SetFlags(RESIZE_WIDTH|RESIZE_HEIGHT, OP_OR);
}

ScrollView::~ScrollView()
{
	View::RemoveSubview(&contentView); // no delete
}

void ScrollView::SizeChanged(const Size&)
{

}

void ScrollView::AddSubviewInFrontOfView(View* front, const View* back)
{
	contentView.AddSubviewInFrontOfView(front, back);
}

View* ScrollView::RemoveSubview(const View* view)
{
	return contentView.RemoveSubview(view);
}

void ScrollView::Scroll(const Point& p)
{
	// FIXME: this needs to be limited. Need to implement the resize flags...
	Point newOrigin = p + contentView.Origin();
	contentView.SetFrameOrigin(newOrigin);
}

bool ScrollView::OnKeyPress(const KeyboardEvent& key, unsigned short /*mod*/)
{
	// TODO: get scroll amount from settings
	int amount = 10;
	Point scroll;
	switch (key.keycode) {
		case GEM_UP:
			scroll.y = amount;
			break;
		case GEM_DOWN:
			scroll.y = -amount;
			break;
		case GEM_LEFT:
			scroll.x = amount;
			break;
		case GEM_RIGHT:
			scroll.x = -amount;
			break;
	}
	if (!scroll.isnull()) {
		Scroll(scroll);
		return true;
	}

	return false;
}

void ScrollView::OnMouseWheelScroll(short x, short y)
{
	Scroll(Point(x, y));
}

}
