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

using SelectOption = std::pair<int, String>;
using OptionId_t = EventMgr::TapMonitorId;
using Option_t = size_t;

static const Color SelectOptionHover(255, 180, 0, 255);  // default hover color for SelectOption
static const Color SelectOptionSelected(55, 100, 0, 255);// default selected color for SelectOption

/**
 * @class TextArea
 * Widget capable of displaying long paragraphs of text.
 * It is usually scrolled with a ScrollBar widget
 */

class GEM_EXPORT TextArea final : public Control, public View::Scrollable {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(const Region& drawFrame, const Region& clip) override;
	
	class OptionContext {
		OptionId_t id = -1;
		Option_t idx = -1;
		
	public:
		OptionContext() noexcept = default;
		OptionContext(OptionId_t id, Option_t opt) noexcept
		: id(id), idx(opt) {}
		
		bool operator==(const OptionContext& rhs) const noexcept {
			return id == rhs.id && idx == rhs.idx;
		}
		
		bool operator!=(const OptionContext& rhs) const noexcept {
			return !operator==(rhs);
		}
	};
	
	class SpanSelector : public ContentContainer {
		struct OptSpan : public TextContainer {
			OptSpan(const Region& frame, Holder<Font> font, const Color& fg, const Color& bg)
				: TextContainer(frame, std::move(font))
			{
				SetColors(fg, bg);
			}
			
			// forward OnMouseLeave to superview (SpanSelector) as a mouse over
			void OnMouseLeave(const MouseEvent& me, const DragOp*) override {
				assert(superView);
				superView->MouseOver(me);
			}

			bool Editable() const override { return false; }
		};
	private:
		TextArea& ta;
		TextContainer* hoverSpan = nullptr;
		TextContainer* selectedSpan = nullptr;
		size_t size;
		Option_t selected = -1;
		EventMgr::TapMonitorId id;

	private:
		void ClearHover();
		TextContainer* TextAtPoint(const Point&);
		TextContainer* TextAtIndex(Option_t idx);

		bool OnMouseOver(const MouseEvent& /*me*/) override;
		bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod) override;
		void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) override;

		bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;
		bool KeyEvent(const Event& event);

		void SizeChanged(const Size&) override;

		bool Editable() const { return false; }

	public:
		// FIXME: we get messed up is SetMargin is called. there is no notification that they have changed and so our subviews are overflowing.
		// working around that by passing them in the ctor, but its a poor fix.
		SpanSelector(TextArea& ta, const std::vector<const String*>&, bool numbered, ContentContainer::Margin m = Margin());
		~SpanSelector() override;

		size_t NumOpts() const { return size;};
		void MakeSelection(Option_t idx);
		TextContainer* Selection() const { return selectedSpan; }
		Option_t SelectionIdx() const { return selected; }
		OptionContext Context() const { return OptionContext(id, selected); }
		
		bool CanLockFocus() const override { return false; }
	};

public:
	enum COLOR_TYPE {
		COLOR_NORMAL = 0,	// standard text color
		COLOR_INITIALS,	// color for finit. used only is some cases.
		COLOR_BACKGROUND, // the background color for all text
		COLOR_OPTIONS,	// normal palette for selectable options (dialog/listbox)
		COLOR_HOVER,	// color for hovering options (dialog/listbox)
		COLOR_SELECTED,	// selected list box/dialog option.
		
		COLOR_TYPE_COUNT
	};

	TextArea(const Region& frame, Holder<Font> text);
	TextArea(const Region& frame, Holder<Font> text, Holder<Font> caps);
	
	~TextArea() final;

	bool IsOpaque() const override { return false; }
	
	void SetColor(const Color&, COLOR_TYPE);

	/** Sets the Actual Text */
	void SetText(String text) override;
	/** Clears the textarea */
	void ClearText();

	/** Appends a String to the current Text */
	void AppendText(String text);
	/** Inserts a String into the current Text at pos */
	// int InsertText(const char* text, int pos);

	/** Per Pixel scrolling */
	void ScrollDelta(const Point& p) override;
	void ScrollTo(const Point& p) override;
	void ScrollToY(int y, ieDword lineduration = 0);
	int ContentHeight() const;

	int LineCount() const;
	int LineHeight() const;

	void SetScrollbar(ScrollBar*);
	void SetSelectOptions(const std::vector<SelectOption>&, bool numbered);
	
	void SelectAvailableOption(Option_t idx);
	/** Set Selectable */
	void SetSelectable(bool val);
	void SetSpeakerPicture(Holder<Sprite2D> Picture);

	ContentContainer::Margin GetMargins() const;
	void SetMargins(ContentContainer::Margin m);

	/** Returns the selected text */
	String QueryText() const override;
	/** Marks textarea for redraw with a new value */
	void UpdateState(value_t opt) override;
	void DidFocus() override;
	void DidUnFocus() override;
	
	void AddSubviewInFrontOfView(View*, const View* = NULL) override;

private: // Private attributes
	// dialog and listbox handling
	std::vector<value_t> values;
	const Content* dialogBeginNode;
	Holder<Sprite2D> speakerPic;
	// dialog options container
	SpanSelector* selectOptions = nullptr;
	OptionContext optionContext;
	// standard text display container
	TextContainer* textContainer = nullptr;
	ScrollView scrollview;
	Timer* historyTimer = nullptr;

	/** Fonts */
	Holder<Font> finit, ftext;
	GemMarkupParser parser;
	ContentContainer::Margin textMargins;

	Color colors[COLOR_TYPE_COUNT] = {ColorWhite};
	Holder<Palette> invertedText;

private: //internal functions
	void SetColor(const Color*, COLOR_TYPE);

	void UpdateScrollview();
	Region UpdateTextFrame();
	void UpdateStateWithSelection(Option_t optIdx);
	void SizeChanged(const Size&) override { UpdateScrollview(); }
	void FlagsChanged(unsigned int /*oldflags*/) override;

	int TextHeight() const;
	int OptionsHeight() const;

	void TrimHistory(size_t lines);
	void TextChanged(const TextContainer& tc);
	void ClearHistoryTimer();

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
