/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

/**
 * @file Button.h
 * Declares Button widget, for displaying buttons in the GUI
 * @author GemRB Development Team
 */


#ifndef BUTTON_H
#define BUTTON_H

#include "GUI/GUIAnimation.h"
#include "GUI/Control.h"
#include "GUI/TextSystem/Font.h"
#include "Sprite2D.h"

#include "exports.h"

#include <vector>

namespace GemRB {

class Palette;
using PaletteHolder = Holder<Palette>;

// NOTE: keep these synchronized with GUIDefines.py!!!
#define IE_GUI_BUTTON_UNPRESSED 0
#define IE_GUI_BUTTON_PRESSED   1
#define IE_GUI_BUTTON_SELECTED  2
#define IE_GUI_BUTTON_DISABLED  3
// Like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap
#define IE_GUI_BUTTON_LOCKED    4
// Draws the disabled bitmap, but otherwise works like unpressed
#define IE_GUI_BUTTON_FAKEDISABLED     5
#define IE_GUI_BUTTON_FAKEPRESSED    6
#define IE_GUI_BUTTON_LOCKED_PRESSED    7  //all the same as LOCKED

#define IE_GUI_BUTTON_NO_IMAGE     0x00000001   // don't draw image (BAM)
#define IE_GUI_BUTTON_PICTURE      0x00000002   // draw picture (BMP, MOS, ...)
#define IE_GUI_BUTTON_SOUND        0x00000004
#define IE_GUI_BUTTON_CAPS         0x00000008   // convert text to uppercase
#define IE_GUI_BUTTON_CHECKBOX     0x00000010   // or radio button
#define IE_GUI_BUTTON_RADIOBUTTON  0x00000020   // sticks in a state
#define IE_GUI_BUTTON_ANIMATED     0x00000080

//these bits are hardcoded in the .chu structure
#define IE_GUI_BUTTON_ALIGN_LEFT   0x00000100
#define IE_GUI_BUTTON_ALIGN_RIGHT  0x00000200
#define IE_GUI_BUTTON_ALIGN_TOP    0x00000400
#define IE_GUI_BUTTON_ALIGN_BOTTOM 0x00000800
#define IE_GUI_BUTTON_ALIGNMENT_FLAGS (IE_GUI_BUTTON_ALIGN_LEFT|IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_TOP|IE_GUI_BUTTON_ALIGN_BOTTOM)
#define IE_GUI_BUTTON_ANCHOR       0x00001000 //not implemented yet
#define IE_GUI_BUTTON_LOWERCASE    0x00002000
//#define IE_GUI_BUTTON_MULTILINE    0x00004000 // don't set the single line flag
//end of hardcoded part
#define IE_GUI_BUTTON_NO_TEXT      0x00010000   // don't draw button label
#define IE_GUI_BUTTON_PLAYRANDOM   0x00020000
#define IE_GUI_BUTTON_PLAYONCE     0x00040000
#define IE_GUI_BUTTON_PLAYALWAYS   0x00080000   // play even when game is paused

#define IE_GUI_BUTTON_CENTER_PICTURES 0x00100000 // center button's PictureList
#define IE_GUI_BUTTON_BG1_PAPERDOLL   0x00200000 // BG1-style paperdoll PictureList
#define IE_GUI_BUTTON_HORIZONTAL      0x00400000 // horizontal clipping of overlay
#define IE_GUI_BUTTON_NO_TOOLTIP      0x00800000

//composite button flags
#define IE_GUI_BUTTON_NORMAL       0x00000004   // default button, doesn't stick
#define IE_GUI_BUTTON_PORTRAIT     (IE_GUI_BUTTON_PLAYONCE|IE_GUI_BUTTON_PLAYALWAYS|IE_GUI_BUTTON_PICTURE)   // portrait

/** Border/frame settings for a button */
struct ButtonBorder {
	Region rect;
	Color color;
	bool filled;
	bool enabled;
};

#define MAX_NUM_BORDERS 3

enum BUTTON_IMAGE_TYPE {
	BUTTON_IMAGE_NONE = -1,
	BUTTON_IMAGE_UNPRESSED,
	BUTTON_IMAGE_PRESSED,
	BUTTON_IMAGE_SELECTED,
	BUTTON_IMAGE_DISABLED,
	BUTTON_IMAGE_TYPE_COUNT
};

/**
 * @class Button
 * Button widget, used mainly for buttons, but also for PixMaps (static images)
 * or for Toggle Buttons.
 */

class GEM_EXPORT Button : public Control {
public:
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action DragDrop = ACTION_CUSTOM(0); // drag and drop complete
	};

	struct PortraitDragOp : public DragOp {
		const ieDword PC;

		PortraitDragOp(Button* b)
		: DragOp(b), PC(b->ControlID + 1) {}
	};

public:
	Button(Region& frame);
	~Button();

	bool IsAnimated() const;
	bool IsOpaque() const;
	/** Sets the 'type' Image of the Button to 'img'.
	see 'BUTTON_IMAGE_TYPE' */
	void SetImage(BUTTON_IMAGE_TYPE, Sprite2D* img);
	/** Sets the Button State */
	void SetState(unsigned char state);
	/** Sets the Text of the current control */
	void SetText(const String& string);
	/** Sets the Picture */
	void SetPicture(Sprite2D* Picture);
	/** Clears the list of Pictures */
	void ClearPictureList();
	/** Add picture to the end of the list of Pictures */
	void StackPicture(Sprite2D* Picture);
	/** Sets border/frame parameters */
	void SetBorder(int index, const Region&, const Color &color, bool enabled = false, bool filled = false);
	/** Sets horizontal overlay, used in portrait hp overlay */
	void SetHorizontalOverlay(double clip, const Color &src, const Color &dest);
	/** Sets font used for drawing button label */
	void SetFont(Font* newfont);
	/** Enables or disables specified border/frame */
	void EnableBorder(int index, bool enabled);

	String QueryText() const { return Text; }
	String TooltipText() const;

	Holder<DragOp> DragOperation();
	bool AcceptsDragOperation(const DragOp&) const;
	void CompleteDragOperation(const DragOp&);

	Sprite2D* Cursor() const;

	/** Refreshes the button from a radio group */
	void UpdateState(unsigned int Sum);
	/** Set palette used for drawing button label in normal state.  */
	void SetTextColor(const Color &fore, const Color &back);
	/** Sets percent (0-1.0) of width for clipping picture */
	void SetPictureClipping(double clip)  { Clipping = clip; }
	/** Set explicit anchor point for text */
	void SetAnchor(ieWord x, ieWord y);
	/** Set offset pictures and label move when button is pressed */
	void SetPushOffset(ieWord x, ieWord y);

	bool SetHotKey(KeyboardKey key, short mod = 0, bool global = false);
	//KeyboardKey GetHotKey() { return hotKey.key; }

private: // Private attributes
	String Text;
	bool hasText;
	Font* font;
	bool ToggleState;
	bool pulseBorder;
	PaletteHolder normal_palette;
	PaletteHolder disabled_palette;
	Sprite2D* buttonImages[BUTTON_IMAGE_TYPE_COUNT];
	/** Pictures to Apply when the hasPicture flag is set */
	Sprite2D* Picture;
	/** If non-empty, list of Pictures to draw when hasPicture is set */
	std::vector<Sprite2D*> PictureList;
	/** The current state of the Button */
	unsigned char State;
	double Clipping;
	Point drag_start;
	/** HP Bar over portraits */
	ColorAnimation overlayAnim;
	/** Explicit text anchor point if IE_GUI_BUTTON_ANCHOR is set */
	Point Anchor;
	/** Offset pictures and label move when the button is pressed. */
	Point PushOffset;
	/** frame settings */
	ButtonBorder borders[MAX_NUM_BORDERS]{};

	EventMgr::EventCallback HotKeyCallback;

	struct HotKey {
		KeyboardKey key = '\0';
		short mod = 0;
		bool global = false;

		operator bool() const {
			return key != '\0';
		}
	} hotKey;

	void UnregisterHotKey();

	bool HandleHotKey(const Event&);
	bool HitTest (const Point&) const;
	void DoToggle();

	void WillDraw();
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);
	
protected:
	/** Mouse Enter */
	void OnMouseEnter(const MouseEvent& /*me*/, const DragOp*);
	/** Mouse Leave */
	void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*);
	/** Mouse Over */
	bool OnMouseOver(const MouseEvent&);
	/** Mouse Button Down */
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod);
	bool OnMouseDrag(const MouseEvent& /*me*/);
	/** Mouse Button Up */
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod);
};

}

#endif
