/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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

#ifndef __GemRB__View__
#define __GemRB__View__

#include "Region.h"

#include <list>

namespace GemRB {

class ScrollBar;

class View {
private:
	View* superView;
	mutable bool dirty;
protected:
	Region frame;
	std::list<View*> subViews;

private:
	void DrawSubviews();

protected:
	virtual void DrawSelf(Region drawFrame, const Region& clip)=0;

	virtual void AddedToView(View*) {}
	virtual void RemovedFromView(View*) {}
	virtual void SubviewAdded(View*) {}
	virtual void SubviewRemoved(View*) {}

	inline Point ConvertPointToSuper(const Point&) const;
	inline Point ConvertPointFromSuper(const Point&) const;

public:
	View(const Region& frame);
	virtual ~View();

	void Draw();

	void MarkDirty();
	bool NeedsDraw() const;

	virtual bool IsAnimated() const { return false; }

	Region Frame() const { return frame; }
	Point Origin() const { return frame.Origin(); }
	Size Dimensions() const { return frame.Dimensions(); }
	void SetFrame(const Region& r);
	void SetFrameOrigin(const Point&);
	void SetFrameSize(const Size&);

	void AddSubviewInFrontOfView(View*, const View* = NULL);
	View* RemoveSubview(const View*);
	View* SubviewAt(const Point&, bool ignoreTransparency = false);

};

}

#endif /* defined(__GemRB__View__) */
