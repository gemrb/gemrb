/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2014 The GemRB Project
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
 */

#ifndef TEXTCONTAINER_H
#define TEXTCONTAINER_H

#include "Font.h"
#include "Region.h"

#include "GUI/View.h"
#include "Strings/String.h"

#include <deque>
#include <utility>

namespace GemRB {

class Font;
class Palette;
class Sprite2D;
class ContentContainer;
class TextContainer;

struct LayoutRegion {
	Region region;
	
	explicit LayoutRegion(Region r)
	: region(std::move(r)) {}
};

using LayoutRegions = std::vector<std::shared_ptr<LayoutRegion>>;

// interface for both content and content containers
// can instantiate to produce empty areas in layout
class Content {
friend class ContentContainer;
protected:
	// content doesn't have an x, y (-1, -1) unless we want absolute positioning
	// same applies for the dimensions, 0x0 implies unlimited area
	Region frame; // TODO: origin currently unused
	ContentContainer* parent = nullptr;

public:
	explicit Content(const Size& size) : frame(Point(0, 0), size) {};
	virtual ~Content() noexcept = default;

	virtual Size ContentFrame() const { return frame.size; };

protected:
	// point is relative to Region. Region is a screen region.
	virtual void DrawContentsInRegions(const LayoutRegions&, const Point&) const {};
	virtual LayoutRegions LayoutForPointInRegion(Point p, const Region&) const;
};


// Content classes
class TextSpan final : public Content
{
friend class TextContainer;
private:
	String text;
	const Holder<Font> font;
	Font::PrintColors* colors = nullptr;
	
	struct TextLayoutRegion : LayoutRegion {
		size_t beginCharIdx;
		size_t endCharIdx;
		
		TextLayoutRegion(Region r, size_t begin, size_t end)
		: LayoutRegion(std::move(r)), beginCharIdx(begin), endCharIdx(end) {}
	};

public:
	// make a "block" of text that always occupies the area of "size", or autosizes if size in NULL
	// TODO: we should probably be able to align the text in the frame
	TextSpan(String string, const Holder<Font> font, const Size* = nullptr);
	TextSpan(String string, const Holder<Font> font, Font::PrintColors cols, const Size* = nullptr);
	TextSpan(const TextSpan&) = delete;
	~TextSpan() final;
	TextSpan& operator=(const TextSpan&) = delete;
	
	void ClearColors();
	void SetColors(const Color& fg, const Color& bg);

	const String& Text() const { return text; };

	unsigned char Alignment = IE_FONT_ALIGN_LEFT;

protected:
	void DrawContentsInRegions(const LayoutRegions&, const Point&) const override;
	LayoutRegions LayoutForPointInRegion(Point p, const Region&) const override;

private:
	inline const Holder<Font> LayoutFont() const;
	inline Region LayoutInFrameAtPoint(const Point&, const Region&) const;
};


class ImageSpan : public Content
{
private:
	Holder<Sprite2D> image;

public:
	explicit ImageSpan(const Holder<Sprite2D>& image);

protected:
	void DrawContentsInRegions(const LayoutRegions&, const Point&) const override;
};


// Content container classes
class ContentContainer : public View
{
public:
	using ContentList = std::list<Content*>;

	struct Margin {
		ieByte top;
		ieByte right;
		ieByte bottom;
		ieByte left;

		Margin() : top(0), right(0), bottom(0), left(0) {}

		explicit Margin(ieByte top)
		: top(top), right(top), bottom(top), left(top) {}

		Margin(ieByte top, ieByte right)
		: top(top), right(right), bottom(top), left(right) {}

		Margin(ieByte top, ieByte right, ieByte bottom)
		: top(top), right(right), bottom(bottom), left(right) {}

		Margin(ieByte top, ieByte right, ieByte bottom, ieByte left)
		: top(top), right(right), bottom(bottom), left(left) {}
	};

protected:
	ContentList contents;

	struct Layout {
		const Content* content;
		LayoutRegions regions;
		
		Layout(const Content* c, LayoutRegions rgns)
		: content(c), regions(std::move(rgns)) {
			assert(!regions.empty());
		}

		bool operator==(const Content* c) const {
			return c == content;
		}

		bool operator<(const Point& p) const {
			const Region& r = regions.back()->region;
			return r.y < p.y || (r.x < p.x && r.y == p.y);
		}

		bool PointInside(const Point& p) const {
			for (const auto& layoutRegion : regions) {
				const Region r = layoutRegion->region;
				if (r.PointInside(p)) {
					return true;
				}
			}
			return false;
		}
	};

	using ContentLayout = std::deque<Layout>;
	ContentLayout layout;
	Point layoutPoint;

	Margin margin;

public:
	explicit ContentContainer(const Region& frame);
	~ContentContainer() override;

	// append a container to the end of the container. The container takes ownership of the span.
	virtual void AppendContent(Content* content);
	// Insert a span to a new position in the list. The container takes ownership of the span.
	virtual void InsertContentAfter(Content* newContent, const Content* existing);
	// removes the span from the container and transfers ownership to the caller.
	// Returns a non-const pointer to the removed span.
	Content* RemoveContent(const Content* content);
	virtual void DeleteContentsInRect(const Region&);

	Content* ContentAtPoint(const Point& p) const;
	const ContentList& Contents() const { return contents; }

	const Region* ContentRegionForRect(const Region& rect) const;
	Region BoundingBoxForContent(const Content*) const;
	Region BoundingBoxForLayout(const LayoutRegions&) const;
	
	void SetMargin(Margin m);
	void SetMargin(ieByte top, ieByte right, ieByte bottom, ieByte left);
	inline void SetMargin(ieByte top, ieByte right, ieByte bottom) { SetMargin(top, right, bottom, right); }
	inline void SetMargin(ieByte top, ieByte right) { SetMargin(top, right, top, right); }
	inline void SetMargin(ieByte top) { SetMargin(top, top, top, top); }

protected:
	void SubviewAdded(View* view, View* parent) override;
	void LayoutContentsFrom(ContentList::const_iterator);
	void LayoutContentsFrom(const Content*);
	Content* RemoveContent(const Content* content, bool doLayout);
	ContentList::iterator EraseContent(ContentList::iterator it);
	ContentList::iterator EraseContent(ContentList::iterator beg, ContentList::iterator end);

	const Layout& LayoutForContent(const Content*) const;
	const Layout* LayoutAtPoint(const Point& p) const;

	void DrawSelf(const Region& drawFrame, const Region& clip) override;
	virtual void DrawContents(const Layout& contentLayout, Point point);
	
	void SizeChanged(const Size& oldSize) override;

private:
	virtual void ContentRemoved(const Content* /*content*/) {};
	
	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	void DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
};

// TextContainers can hold any content, but they represent a string of text that is divided into TextSpans
class TextContainer : public ContentContainer {
private:
	using TextLayout = TextSpan::TextLayoutRegion;
	// default font/palette for adding plain text
	Holder<Font> font;
	Font::PrintColors* colors = nullptr;
	unsigned char alignment = IE_FONT_ALIGN_LEFT;

	size_t textLen = 0;
	size_t cursorPos = 0;
	size_t printPos = 0;
	Point cursorPoint;

private:
	String TextFrom(ContentList::const_iterator) const;

	void ContentRemoved(const Content* content) override;

	void MoveCursorToPoint(const Point& p);
	LayoutRegions::const_iterator FindCursorRegion(const Layout&) const;

	void DrawSelf(const Region& drawFrame, const Region& clip) override;
	void DrawContents(const Layout& layout, Point point) override;

	virtual bool Editable() const { return IsReceivingEvents(); }
	void SizeChanged(const Size& oldSize) override;

	using ContentIndex = std::pair<size_t, ContentList::iterator>;
	ContentIndex FindContentForChar(size_t idx);

public:
	TextContainer(const Region& frame, Holder<Font> font);
	TextContainer(const TextContainer&) = delete;
	~TextContainer() override;
	TextContainer& operator=(const TextContainer&) = delete;
	
	// relative to cursor pos
	void InsertText(const String& text);
	void DeleteText(size_t len);

	void AppendText(String text);
	void AppendText(String text, const Holder<Font> fnt, const Font::PrintColors* = nullptr);
	String TextFrom(const Content*) const;
	String Text() const;

	void DidFocus() override;
	void DidUnFocus() override;

	void ClearColors();
	void SetColors(const Color& fg, const Color& bg);
	const Font::PrintColors* TextColors() const { return colors; }
	void SetFont(Holder<Font> fnt) { font = fnt; }
	const Holder<Font> TextFont() const { return font; }
	void SetAlignment(unsigned char align) { alignment = align; }

	void CursorHome();
	void CursorEnd();
	void AdvanceCursor(int);
	
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) override;
	bool OnMouseDrag(const MouseEvent& /*me*/) override;
	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;
	void OnTextInput(const TextEvent& /*te*/) override;

	using EditCallback = Callback<void, TextContainer&>;
	EditCallback callback;
};

}
#endif
