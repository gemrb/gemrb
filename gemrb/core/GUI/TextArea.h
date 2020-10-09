/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "GUI/Control.h"
#include "GUI/ScrollView.h"
#include "GUI/TextSystem/Font.h"
#include "GUI/TextSystem/GemMarkup.h"
#include "GUI/TextSystem/TextContainer.h"

#include "RGBAColor.h"
#include "exports.h"

#include <vector>

namespace GemRB {

typedef std::pair<int, String> SelectOption;

static const Color SelectOptionHover(255, 180, 0, 0);  // default hover color for SelectOption
static const Color SelectOptionSelected(55, 100, 0, 0);// default selected color for SelectOption

/**
 * @class TextArea
 * Widget capable of displaying long paragraphs of text.
 * It is usually scrolled with a ScrollBar widget
 */

class GEM_EXPORT TextArea : public Control, public View::Scrollable {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);
	
	class SpanSelector : public ContentContainer {
		struct OptSpan : public TextContainer {
			OptSpan(const Region& frame, Font* font, Holder<Palette> pal)
			: TextContainer(frame, font, pal) {}
			
			// forward OnMouseLeave to superview (SpanSelector) as a mouse over
			void OnMouseLeave(const MouseEvent& me, const DragOp*) {
				assert(superView);
				superView->MouseOver(me);
			}

			bool Editable() const { return false; }
		};
	private:
		TextArea& ta;
		TextContainer* hoverSpan, *selectedSpan;
		size_t size;
		EventMgr::TapMonitorId id;

	private:
		void ClearHover();
		TextContainer* TextAtPoint(const Point&);
		TextContainer* TextAtIndex(size_t idx);

		bool OnMouseOver(const MouseEvent& /*me*/);
		bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod);
		void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*);

		bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);
		bool KeyEvent(const Event& event);

		void SizeChanged(const Size&);

		bool Editable() const { return false; }

	public:
		// FIXME: we get messed up is SetMargin is called. there is no notification that they have changed and so our subviews are overflowing.
		// working around that by passing them in the ctor, but its a poor fix.
		SpanSelector(TextArea& ta, const std::vector<const String*>&, bool numbered, ContentContainer::Margin m = Margin());
		~SpanSelector();

		size_t NumOpts() const { return size;};
		void MakeSelection(size_t idx);
		TextContainer* Selection() const { return selectedSpan; }
		
		bool CanLockFocus() const { return false; }
	};

public:
	TextArea(const Region& frame, Font* text);
	TextArea(const Region& frame, Font* text, Font* caps,
			 Color hitextcolor, Color initcolor, Color lowtextcolor);

	bool IsOpaque() const { return false; }

	/** Sets the Actual Text */
	void SetText(const String& text);
	/** Clears the textarea */
	void ClearText();

	/** Appends a String to the current Text */
	void AppendText(const String& text);
	/** Inserts a String into the current Text at pos */
	// int InsertText(const char* text, int pos);

	/** Per Pixel scrolling */
	void ScrollDelta(const Point& p);
	void ScrollTo(const Point& p);
	void ScrollToY(int y, ieDword lineduration = 0);
	int ContentHeight() const;

	ieDword LineCount() const;
	ieWord LineHeight() const;

	void SetSelectOptions(const std::vector<SelectOption>&, bool numbered,
						  const Color* color, const Color* hiColor, const Color* selColor);
	
	void SelectAvailableOption(size_t idx);
	/** Set Selectable */
	void SetSelectable(bool val);
	void SetAnimPicture(Holder<Sprite2D> Picture);

	ContentContainer::Margin GetMargins() const;
	void SetMargins(ContentContainer::Margin m);

	/** Returns the selected text */
	String QueryText() const;
	/** Marks textarea for redraw with a new value */
	void UpdateState(unsigned int optIdx);
	void DidFocus();
	void DidUnFocus();

private: // Private attributes
	// dialog and listbox handling
	std::vector<ieDword> values;
	const Content* dialogBeginNode;
	// dialog options container
	SpanSelector* selectOptions;
	// standard text display container
	TextContainer* textContainer;
	ScrollView scrollview;
	Timer* historyTimer;

	/** Fonts */
	Font* finit, * ftext;
	GemMarkupParser parser;
	ContentContainer::Margin textMargins;

	enum PALETTE_TYPE {
		PALETTE_NORMAL = 0,	// standard text color
		PALETTE_OPTIONS,	// normal palette for selectable options (dialog/listbox)
		PALETTE_HOVER,		// palette for hovering options (dialog/listbox)
		PALETTE_SELECTED,	// selected list box/dialog option.
		PALETTE_INITIALS,	// palette for finit. used only is some cases.

		PALETTE_TYPE_COUNT
	};
	Holder<Palette> palettes[PALETTE_TYPE_COUNT];

private: //internal functions
	void Init();
	void SetPalette(const Color*, PALETTE_TYPE);

	void UpdateScrollview();
	Region UpdateTextFrame();
	void SizeChanged(const Size&) { UpdateScrollview(); }
	void FlagsChanged(unsigned int /*oldflags*/);

	int TextHeight() const;
	int OptionsHeight() const;

	void TrimHistory(size_t lines);
	void TextChanged(TextContainer& tc);

public: //Events
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action Change = Control::ValueChange; // text change event (keyboard, etc)
		static const Control::Action Select = ACTION_CUSTOM(0); // selection event such as dialog or a listbox
	};

	enum TextAreaFlags {
		// !!! Keep these synchronized with GUIDefines.py !!!
		AutoScroll = 1,   // TextArea will automatically scroll when new text is appended
		ClearHistory = 2, // TextArea will automatically purge old data as new data is added
		Editable = 4      // TextArea text is editable
	};

	void ClearSelectOptions();
};

}

#endif
