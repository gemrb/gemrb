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

#include "TextArea.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Variables.h"
#include "GUI/EventMgr.h"
#include "GUI/TextSystem/GemMarkup.h"
#include "GUI/Window.h"

#define EDGE_PADDING 3

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text)
	: Control(frame), contentWrapper(Size(frame.w, 0)), ftext(text), palettes()
{
	palette = text->GetPalette();
	finit = ftext;
	Init();
}

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color textcolor, Color initcolor, Color lowtextcolor)
	: Control(frame), contentWrapper(Size(frame.w, 0)), ftext(text), palettes()
{
	palettes[PALETTE_NORMAL] = new Palette( textcolor, lowtextcolor );
	palette = palettes[PALETTE_NORMAL];

	// quick font optimization (prevents creating unnecessary cap spans)
	finit = (caps != ftext) ? caps : ftext;

	if (finit->Baseline < ftext->LineHeight) {
		// FIXME: initcolor is only used for *some* initial fonts
		// this is a hack to workaround the INITIALS font getting its palette set
		// do we have another (more sane) way to tell if a font needs this palette? (something in the BAM?)
		SetPalette(&initcolor, PALETTE_INITIALS);
	} else {
		palettes[PALETTE_INITIALS] = finit->GetPalette();
	}

	Init();
}

void TextArea::Init()
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	starttime = 0;
	strncpy(VarName, "Selected", sizeof(VarName));

	ResetEventHandler( TextAreaOnChange );
	ResetEventHandler( TextAreaOnSelect );

	selectOptions = NULL;
	textContainer = NULL;

	// initialize the Text containers
	SetScrollBar(NULL);
	ClearSelectOptions();
	ClearText();
	SetAnimPicture(NULL);
}

TextArea::~TextArea(void)
{
	for (int i=0; i < PALETTE_TYPE_COUNT; i++) {
		gamedata->FreePalette( palettes[i] );
	}
}

bool TextArea::NeedsDraw()
{
	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		if (TextYPos > textContainer->ContentFrame().h) {
			 // the text is offscreen
			return false;
		}
		// must mark dirty to invalidate the window BG
		MarkDirty();
		return true;
	}

	if (animationEnd) {
		if (TextYPos > textContainer->ContentFrame().h) {
			// the text is offscreen, this happens with chapter text
			ScrollToY(TextYPos); // reset animation values
			return false;
		}
		// update animation for this next draw cycle
		unsigned long curTime = GetTickCount();
		if (animationEnd.time > curTime) {
			//double animProgress = curTime / animationEnd;
			int deltaY = animationEnd.y - animationBegin.y;
			unsigned long deltaT = animationEnd.time - animationBegin.time;
			int y = deltaY * ((double)(curTime - animationBegin.time) / deltaT);
			TextYPos = animationBegin.y + y;
			Owner->InvalidateForControl(this);
		} else if (animationEnd.y != TextYPos) {
			UpdateScrollbar();
			int tmp = animationEnd.y; // FIXME: sidestepping a rounding issue (probably in Scrollbar)
			ScrollToY(animationEnd.y);
			TextYPos = tmp;
			// FIXME: hack due to ScrollToY invalidating window BG
			// we need to ensure another draw cycle
			animationEnd = AnimationPoint(TextYPos, curTime);
		} else {
			animationEnd = AnimationPoint();
		}
		return true;
	}
	return Control::NeedsDraw();
}

void TextArea::DrawInternal(Region& clip)
{
	if (AnimPicture) {
		// speaker portrait
		core->GetVideoDriver()->BlitSprite(AnimPicture, clip.x, clip.y + EDGE_PADDING, true);
		clip.x += AnimPicture->Width + EDGE_PADDING;
	}
	clip.x += EDGE_PADDING;

	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		unsigned long thisTime = GetTickCount();
		if (thisTime>starttime) {
			// ticks is the number of ticks it takes to scroll this font 1 px
			unsigned long ticks = 2400 / ftext->LineHeight;
			starttime = thisTime+ticks;
			TextYPos++;// can't use ScrollToY
		}
	}

	clip.y -= TextYPos;
	contentWrapper.Draw(clip.Origin());

	if (selectOptions) {
		// This hack is to refresh the mouse cursor so that option below cursor gets
		// highlighted during a dialog
		core->GetEventMgr()->FakeMouseMove();
	}
}

void TextArea::SetAnimPicture(Sprite2D* pic)
{
	// FIXME: this behavior really needs to also happen when the TA dimensions change
	// we currntly do that by setting *public* ivars in Control, instead of having a SetSize type method

	// FIXME: we always have to accept NULL because sometimes the control size gets changed after this is called
	// dialog is the only thing that uses an actual picture, so we can safely bail out in that case
	if (pic == AnimPicture && pic != NULL) return;

	Size frame(Width, 0);
	// apply padding to the clip
	frame.w -= (sb) ? EDGE_PADDING : EDGE_PADDING * 2;

	if (pic) {
		int offset = pic->Width + EDGE_PADDING;
		// FIXME: in the original dialog is always indented (even without portrait), I doubt we care, but mentioning it here.
		frame.w -= offset;
	}
	// FIXME: content containers should support the "flexible" idiom so we can resize children by resizing parent
	textContainer->SetFrame(Region(Point(), frame));
	contentWrapper.SetFrame(Region(Point(), frame));

	Control::SetAnimPicture(pic);
}

void TextArea::UpdateScrollbar()
{
	if (sb == NULL) return;

	int textHeight = contentWrapper.ContentFrame().h;
	Region nodeBounds;
	if (dialogBeginNode) {
		// possibly add some phony height to allow dialogBeginNode to the top when the scrollbar is at the bottom
		// add the height of a newline too so that there is a space
		nodeBounds = textContainer->BoundingBoxForContent(dialogBeginNode);
		Size selectFrame = selectOptions->ContentFrame();
		// page = blank line + dialog node + blank line + select options
		int pageH = ftext->LineHeight*2 + nodeBounds.h + selectFrame.h;
		if (pageH < Height) {
			// if the node isnt a full page by itself we need to fake it
			textHeight += Height - pageH;
		}
	}
	int rowHeight = GetRowHeight();
	int newRows = (textHeight + rowHeight - 1) / rowHeight; // round up
	if (newRows != rows) {
		rows = newRows;
		ScrollBar* bar = ( ScrollBar* ) sb;
		ieWord visibleRows = (Height / GetRowHeight());
		ieWord sbMax = (rows > visibleRows) ? (rows - visibleRows) : 0;
		bar->SetMax(sbMax);
	}
	if (Flags&IE_GUI_TEXTAREA_AUTOSCROLL
		&& dialogBeginNode) {
		// now scroll dialogBeginNode to the top less a blank line
		ScrollToY(nodeBounds.y - ftext->LineHeight);
	}
}

/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Text Area Control. */
int TextArea::SetScrollBar(Control* ptr)
{
	ScrollBar* bar = (ScrollBar*)ptr;
	Control::SetScrollBar(bar);
	// we need to update the ScrollBar position based around TextYPos
	rows = 0; // force an update in UpdateScrollbar()
	UpdateScrollbar();
	if (Flags&IE_GUI_TEXTAREA_AUTOSCROLL) {
		bar->SetPos(bar->Value); // scroll to the bottom
	} else {
		// seems silly to call ScrollToY() with the current position,
		// but it is to update the scrollbar so not a mistake
		ScrollToY(TextYPos);
	}
	return (bool)sb;
}

/** Sets the Actual Text */
void TextArea::SetText(const String& text)
{
	ClearText();
	AppendText(text);
}

void TextArea::SetPalette(const Color* color, PALETTE_TYPE idx)
{
	assert(idx < PALETTE_TYPE_COUNT);
	if (color) {
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = new Palette( *color, ColorBlack );
	} else if (idx > PALETTE_NORMAL) {
		// default to normal
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = palettes[PALETTE_NORMAL];
		palettes[idx]->acquire();
	}
}

void TextArea::AppendText(const String& text)
{
	if (Flags&IE_GUI_TEXTAREA_HISTORY) {
		int heightLimit = (ftext->LineHeight * 100); // 100 lines of content
		// start trimming content from the top until we are under the limit.
		Size frame = textContainer->ContentFrame();
		int currHeight = frame.h;
		if (currHeight > heightLimit) {
			Region exclusion(Point(), Size(frame.w, currHeight - heightLimit));
			textContainer->DeleteContentsInRect(exclusion);
		}
	}

	size_t tagPos = text.find_first_of('[');
	if (tagPos != String::npos) {
		// share a single parser for all TextAreas
		static GemMarkupParser parser;
		parser.SetTextDefaults(ftext, finit, palette);
		parser.ParseMarkupStringIntoContainer(text, *textContainer);
	} else if (text.length()) {
		if (finit != ftext) {
			// append cap spans
			size_t textpos = text.find_first_not_of(WHITESPACE_STRING);
			// FIXME: ? maybe we actually want the newlines etc?
			// I think maybe if we clean up the GUIScripts this isn't needed.
			if (textpos != String::npos) {
				// FIXME: initpalette should *not* be used for drop cap font or state fonts!
				// need to figure out how to handle this because it breaks drop caps

				// we must create and append this span here (instead of using AppendText),
				// because the original data files for the DC font specifies a line height of 13
				// that would cause overlap when the lines wrap beneath the DC if we didnt specify the correct size
				Size s = finit->GetGlyph(text[textpos]).size;
				if (s.h > ftext->LineHeight) {
					// pad this only if it is "real" (it is higher than the other text).
					// some text areas have a "cap" font assigned in the CHU that differs from ftext, but isnt meant to be a cap
					// see BG2 chargen
					s.w += EDGE_PADDING;
				}
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, palettes[PALETTE_INITIALS], &s);
				textContainer->AppendContent(dc);
				textpos++;
				// FIXME: assuming we have more text!
				// FIXME: as this is currently implemented, the cap is *not* considered part of the word,
				// there is potential wrapping errors (BG2 char gen).
				// we could solve this by wrapping the cap and the letters remaining letters of the word into their own TextContainer
			} else {
				textpos = 0;
			}
			textContainer->AppendText(text.substr(textpos));
		} else {
			textContainer->AppendText(text);
		}
	}

	if (sb) {
		UpdateScrollbar();
		if (Flags&IE_GUI_TEXTAREA_AUTOSCROLL && !selectOptions)
		{
			// scroll to the bottom
			int bottom = contentWrapper.ContentFrame().h - Height;
			if (bottom > 0)
				ScrollToY(bottom, NULL, 500); // animated scroll
		}
	}
	MarkDirty();
}
/*
int TextArea::InsertText(const char* text, int pos)
{
	// TODO: actually implement this
	AppendText(text);
	return pos;
}
*/
/** Key Press Event */
bool TextArea::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Flags & IE_GUI_TEXTAREA_EDITABLE) {
		if (Key >= 0x20) {
			MarkDirty();

			// TODO: implement this! currently does nothing

			RunEventHandler( TextAreaOnChange );
		}
		return true;
	}

	if (( Key < '1' ) || ( Key > '9' ))
		return false;

	MarkDirty();

	unsigned int lookupIdx = Key - '1';
	if (lookupIdx < OptSpans.size()) {
		UpdateState(VarName, lookupIdx);
	}
	return true;
}

/** Special Key Press */
bool TextArea::OnSpecialKeyPress(unsigned char Key)
{
	size_t len = 0;
	size_t CurPos = 0;

	if (!(Flags&IE_GUI_TEXTAREA_EDITABLE)) {
		return false;
	}
	MarkDirty();
	// TODO: implement text editing. (going to be tricky...)
	switch (Key) {
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_UP:
			break;
		case GEM_DOWN:
			break;
		case GEM_END:
			break;
		case GEM_LEFT:
			if (CurPos > 0) {
				CurPos--;
			} else {

			}
			break;
		case GEM_RIGHT:
			if (CurPos < len) {
				CurPos++;
			} else {

			}
			break;
		case GEM_DELETE:
			if (CurPos>=len) {
				break;
			}
			break;
		case GEM_BACKSP:
			if (CurPos != 0) {
				if (len<1) {
					break;
				}
				CurPos--;
			} else {

			}
			break;
		 case GEM_RETURN:
			//add an empty line after CurLine
			// TODO: implement this
			//copy the text after the cursor into the new line

			//truncate the current line

			//move cursor to next line beginning
			CurPos=0;
			break;
	}
	RunEventHandler( TextAreaOnChange );
	return true;
}

int TextArea::GetRowHeight() const
{
	return ftext->LineHeight;
}

/** Will scroll y pixels. sender is the control requesting the scroll (ie the scrollbar) */
void TextArea::ScrollToY(int y, Control* sender, ieWord duration)
{
	// set up animation if required
	if  (duration) {
		// HACK: notice the values arent clamped here.
		// this is sort of a hack we allow for chapter text so it can begin and end out of sight
		unsigned long startTime = GetTickCount();
		animationBegin = AnimationPoint(TextYPos, startTime);
		animationEnd = AnimationPoint(y, startTime + duration);
		return;
	} else if (animationEnd) {
		// cancel the existing animation (if any)
		animationBegin = AnimationPoint();
		animationEnd = AnimationPoint();
	}

	if (sb && sender != sb) {
		// we must "scale" the pixels
		((ScrollBar*)sb)->SetPosForY(y * (((ScrollBar*)sb)->GetStep()-1 / ftext->LineHeight));
		// sb->SetPosForY will recall this method so we dont need to do more... yet.
	} else if (sb) {
		// our scrollbar has set position for us
		TextYPos = y;
		MarkDirty();
	} else {
		// no scrollbar. need to call SetRow myself.
		// SetRow will set TextYPos.
		SetRow( y / ftext->LineHeight );
	}
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row <= rows) {
		TextYPos = row * GetRowHeight();
		MarkDirty();
	}
}

/** Mousewheel scroll */
/** This method is key to touchscreen scrolling */
void TextArea::OnMouseWheelScroll(short /*x*/, short y)
{
	if (!(IE_GUI_TEXTAREA_SMOOTHSCROLL & Flags)){
		unsigned long fauxY = TextYPos;
		if ((long)fauxY + y <= 0) fauxY = 0;
		else fauxY += y;
		ScrollToY((int)fauxY);
		core->GetEventMgr()->FakeMouseMove();
	}
}

/** Mouse Over Event */
void TextArea::OnMouseOver(unsigned short x, unsigned short y)
{
	if (!selectOptions)
		return;

	TextContainer* span = NULL;
	if (selectOptions) {
		Point p = Point(x, y);
		p.x -= (AnimPicture) ? AnimPicture->Width + EDGE_PADDING : 0;
		p.y -= textContainer->ContentFrame().h - TextYPos;
		// container only has text, so...
		span = dynamic_cast<TextContainer*>(selectOptions->ContentAtPoint(p));
	}

	if (hoverSpan || span)
		MarkDirty();

	ClearHover();
	if (span) {
		hoverSpan = span;
		hoverSpan->SetPalette(palettes[PALETTE_HOVER]);
	}
}

void TextArea::OnMouseDown(unsigned short /*x*/, unsigned short /*y*/, unsigned short Button,
						   unsigned short /*Mod*/)
{

	ScrollBar* scrlbr = (ScrollBar*) sb;

	if (!scrlbr) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl->ControlType == IE_GUI_SCROLLBAR)) {
			scrlbr = (ScrollBar *) ctrl;
		}
	}
	if (scrlbr) {
		switch(Button) {
			case GEM_MB_SCRLUP:
				scrlbr->ScrollUp();
				break;
			case GEM_MB_SCRLDOWN:
				scrlbr->ScrollDown();
				break;
		}
	}
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
						 unsigned short Button, unsigned short /*Mod*/)
{
	if (!(Button & (GEM_MB_ACTION|GEM_MB_MENU)) || !hoverSpan)
		return;

	if (hoverSpan) { // select the item under the mouse
		int optIdx = 0;
		std::vector<OptionSpan>::const_iterator it;
		for (it = OptSpans.begin(); it != OptSpans.end(); ++it) {
			if( (*it).second == hoverSpan ) {
				break;
			}
			optIdx++;
		}
		UpdateState(VarName, optIdx);
	}
}

void TextArea::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	ClearHover();
}

void TextArea::UpdateState(const char* VariableName, unsigned int optIdx)
{
	if (!VariableName[0] || optIdx >= OptSpans.size()) {
		return;
	}
	if (!selectOptions) {
		// no selectable options present
		// set state to safe and return
		ClearSelectOptions();
		return;
	}

	// always run the TextAreaOnSelect handler even if the value hasnt changed
	// the *context* of the value can change (dialog) and the handler will want to know 
	Value = OptSpans[optIdx].first;

	// this can be called from elsewhere (GUIScript), so we need to make sure we update the selected span
	TextContainer* optspan = OptSpans[optIdx].second;
	if (selectedSpan && selectedSpan != optspan) {
		// reset the previous selection
		selectedSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		MarkDirty();
	}
	selectedSpan = optspan;
	selectedSpan->SetPalette(palettes[PALETTE_SELECTED]);

	core->GetDictionary()->SetAt( VarName, Value );
	RunEventHandler(TextAreaOnSelect);
}

String TextArea::QueryText() const
{
	if (selectedSpan) {
		return selectedSpan->Text();
	}
	return textContainer->Text();
}

bool TextArea::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		TextAreaOnChange = handler;
		break;
	case IE_GUI_TEXTAREA_ON_SELECT:
		TextAreaOnSelect = handler;
		break;
	default:
		return false;
	}

	return true;
}

void TextArea::ClearSelectOptions()
{
	OptSpans.clear();
	contentWrapper.RemoveContent(selectOptions);
	delete selectOptions;
	dialogBeginNode = NULL;
	selectOptions = NULL;
	selectedSpan = NULL;
	hoverSpan = NULL;
	// also set the value to "none"
	Value = -1;
	UpdateScrollbar();
}

void TextArea::SetSelectOptions(const std::vector<SelectOption>& opts, bool numbered,
								const Color* color, const Color* hiColor, const Color* selColor)
{
	SetPalette(color, PALETTE_OPTIONS);
	SetPalette(hiColor, PALETTE_HOVER);
	SetPalette(selColor, PALETTE_SELECTED);

	ClearSelectOptions(); // deletes previous options

	Size optFrame(Width - (EDGE_PADDING * 2), 0);
	optFrame.w -= (AnimPicture) ? AnimPicture->Width : 0;
	Size flexFrame(-1, 0); // flex frame for hanging indent after optnum
	selectOptions = new TextContainer(optFrame, ftext, palettes[PALETTE_SELECTED]);

	ContentContainer::ContentList::const_reverse_iterator it = textContainer->Contents().rbegin();
	if (it != textContainer->Contents().rend()) {
		dialogBeginNode = *it; // need to get the last node *before* we append anything
		selectOptions->AppendText(L"\n"); // always want a gap between text and select options for dialog
	}
	for (size_t i = 0; i < opts.size(); i++) {
		TextContainer* selOption = new TextContainer(optFrame, ftext, palettes[PALETTE_OPTIONS]);
		if (numbered) {
			wchar_t optNum[6];
			swprintf(optNum, sizeof(optNum)/sizeof(optNum[0]), L"%d. - ", i+1);
			// TODO: as per the original PALETTE_SELECTED should be updated to the PC color (same color their name is rendered in)
			// but that should probably actually be done by the dialog handler, not here.
			selOption->AppendContent(new TextSpan(optNum, NULL, palettes[PALETTE_SELECTED]));
		}
		selOption->AppendContent(new TextSpan(opts[i].second, NULL, NULL, &flexFrame));

		OptSpans.push_back(std::make_pair(opts[i].first, selOption));

		selectOptions->AppendContent(selOption); // container owns the option
		if (core->GetVideoDriver()->TouchInputEnabled()) {
			// now add a newline for keeping the options spaced out (for touch screens)
			selectOptions->AppendText(L"\n");
		}
	}
	assert(textContainer);

	contentWrapper.InsertContentAfter(selectOptions, textContainer);
	UpdateScrollbar();
	MarkDirty();
}

void TextArea::ClearHover()
{
	if (hoverSpan) {
		if (hoverSpan == selectedSpan) {
			hoverSpan->SetPalette(palettes[PALETTE_SELECTED]);
		} else {
			// reset the old hover span
			hoverSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		}
		hoverSpan = NULL;
	}
}

void TextArea::ClearText()
{
	ClearHover();
	contentWrapper.RemoveContent(textContainer);
	delete textContainer;

	Size frame;
	if (sb) {
		// if we have a scrollbar we should grow as much as needed vertically
		// pad only on left edge
		frame.w = Width - EDGE_PADDING;
	} else {
		// otherwise limit the text to our frame
		// pad on both edges
		frame.w = Width - (EDGE_PADDING * 2);
	}

	textContainer = new TextContainer(frame, ftext, palette);
	contentWrapper.InsertContentAfter(textContainer, NULL); // make sure its at the top

	// reset text position to top
	ScrollToY(0);
	UpdateScrollbar();
}

//setting up the textarea for smooth scrolling, the first
//TEXTAREA_OUTOFTEXT callback is called automatically
void TextArea::SetupScroll()
{
	ClearText();
	TextYPos = -Height; // FIXME: this is somewhat fragile (it is reset by SetRow etc)
	Flags |= IE_GUI_TEXTAREA_SMOOTHSCROLL;
	starttime = GetTickCount();
}

void TextArea::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus && Flags & IE_GUI_TEXTAREA_EDITABLE) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

}
