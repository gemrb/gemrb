// SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __GemRB__ScrollView__
#define __GemRB__ScrollView__

#include "GUI/GUIAnimation.h"
#include "GUI/View.h"

namespace GemRB {
class ScrollBar;

class GEM_EXPORT ScrollView : public View,
			      public View::Scrollable {
	class GEM_EXPORT ContentView : public View {
	private:
		void SizeChanged(const Size& /* oldsize */) override;

		void SubviewAdded(View* view, View* parent) override;
		void SubviewRemoved(View* view, View* parent) override;

		void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
		void DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;

	public:
		using View::View;

		bool CanLockFocus() const override { return false; }
		// TODO: this should be private and happen automatically
		void ResizeToSubviews();
	};

	PointAnimation animation;

	ScrollBar* hscroll = nullptr;
	ScrollBar* vscroll = nullptr;
	Size savedSBSize;

	ContentView contentView;

private:
	void UpdateScrollbars();
	void ScrollbarValueChange(const ScrollBar*);

	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	void DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;

	void FlagsChanged(unsigned int /*oldflags*/) override;
	void ToggleScrollbar(ScrollBar*, bool visible);

protected:
	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;
	bool OnMouseWheelScroll(const Point& delta) override;
	bool OnMouseDrag(const MouseEvent&) override;

public:
	explicit ScrollView(const Region& frame);
	~ScrollView() override;

	void SetVScroll(ScrollBar* sb);
	void SetHScroll(ScrollBar* sb);

	bool IsAnimated() const override { return static_cast<bool>(animation); }

	void AddSubviewInFrontOfView(View*, const View* = NULL) override;
	View* RemoveSubview(const View*);
	View* SubviewAt(const Point&, bool ignoreTransparency = false, bool recursive = false);

	// TODO: this isn't how we want to do things
	// this should happen automatically as subivews are added/removed/resized
	void Update();

	Point ScrollOffset() const;
	void SetScrollIncrement(int);

	// we implement the Scrollable interface so we cant just use a default param
	void ScrollDelta(const Point& p) override;
	void ScrollDelta(const Point& p, ieDword duration);
	void ScrollTo(const Point& p) override;
	void ScrollTo(Point p, ieDword duration);
	bool CanScroll(const Point& p) const;

	Region ContentRegion() const;
};
}
#endif /* __GemRB__ScrollView__ */
