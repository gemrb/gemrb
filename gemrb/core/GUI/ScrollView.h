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

#ifndef __GemRB__ScrollView__
#define __GemRB__ScrollView__

#include "GUI/View.h"

namespace GemRB {
	class ScrollBar;

	class GEM_EXPORT ScrollView : public View {

		class ContentView : public View {
			private:
			void SizeChanged(const Size& /* oldsize */);
			
			public:
			ContentView(const Region& frame)
			: View(frame) {}
			bool CanLockFocus() const { return false; }
		};

		ScrollBar* hscroll;
		ScrollBar* vscroll;

		ContentView contentView;

	private:
		void ScrollDelta(const Point& p);
		void ScrollTo(Point p);

		void ContentSizeChanged(const Size&);
		void ScrollbarValueChange(ScrollBar*);

	public:
		ScrollView(const Region& frame);
		~ScrollView();

		void AddSubviewInFrontOfView(View*, const View* = NULL);
		View* RemoveSubview(const View*);
		View* SubviewAt(const Point&, bool ignoreTransparency = false, bool recursive = false);

		bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);
		void OnMouseWheelScroll(const Point& delta);
		void OnMouseDrag(const MouseEvent&);
	};
}
#endif /* __GemRB__ScrollView__ */
