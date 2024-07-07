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

#include "CHUImporter.h"

#include "RGBAColor.h"

#include "AnimationFactory.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Logging/Logging.h"
#include "GUI/Button.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/Label.h"
#include "GUI/Progressbar.h"
#include "GUI/ScrollBar.h"
#include "GUI/Slider.h"
#include "GUI/TextArea.h"
#include "GUI/TextEdit.h"

using namespace GemRB;

/** This function loads all available windows from the 'stream' parameter. */
bool CHUImporter::Import(DataStream* str)
{
	char Signature[8];
	str->Read(Signature, 8);
	if (strncmp(Signature, "CHUIV1  ", 8) != 0) {
		Log(ERROR, "CHUImporter", "Not a Valid CHU File");
		return false;
	}
	str->ReadDword(WindowCount);
	str->ReadDword(CTOffset);
	str->ReadDword(WEOffset);
	return true;
}

static void GetButton(DataStream* str, Control*& ctrl, const Region& ctrlFrame, bool noImage)
{
	Button* btn = new Button(ctrlFrame);
	ctrl = btn;

	ResRef bamFile;
	ieByte cycle;
	ieByte flagsByte;
	ieByte unpressedIndex;
	ieByte pressedIndex;
	ieByte selectedIndex;
	ieByte disabledIndex;
	ieByte x1;
	ieByte x2;
	ieByte y1;
	ieByte y2;

	str->ReadResRef(bamFile);
	str->Read(&cycle, 1 );
	str->Read(&flagsByte, 1);
	ieDword Flags = static_cast<ieDword>(flagsByte) << 8;
	str->Read(&unpressedIndex, 1);
	str->Read(&x1, 1);
	str->Read(&pressedIndex, 1);
	str->Read(&x2, 1);
	str->Read(&selectedIndex, 1);
	str->Read(&y1, 1);
	str->Read(&disabledIndex, 1);
	str->Read(&y2, 1);

	// Justification comes from the .chu, other bits are set by script
	if (noImage) {
		btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BitOp::OR);
	}
	if (core->HasFeature(GFFlags::UPPER_BUTTON_TEXT)) {
		btn->SetFlags(IE_GUI_BUTTON_CAPS, BitOp::OR);
	}

	btn->SetFlags(Flags, BitOp::OR);
	if (Flags & IE_GUI_BUTTON_ANCHOR) {
		btn->SetAnchor(x1 | (x2 << 8), y1 | (y2 << 8));
	}

	if (bamFile == "guictrl" && unpressedIndex == 0) {
		return;
	}

	auto bam = gamedata->GetFactoryResourceAs<const AnimationFactory>(bamFile, IE_BAM_CLASS_ID);
	if (!bam) {
		Log(ERROR, "CHUImporter", "Cannot Load Button Images, skipping control");
		// iwd2 has fake BAM ResRefs for some Buttons, this will handle bad ResRefs
		return;
	}
	// Cycle is only a byte for buttons
	Holder<Sprite2D> tspr = bam->GetFrame(unpressedIndex, cycle);

	btn->SetImage(ButtonImage::Unpressed, tspr);
	tspr = bam->GetFrame(pressedIndex, cycle);
	btn->SetImage(ButtonImage::Pressed, tspr);
	// work around several controls not setting all the indices
	AnimationFactory::index_t cycleSize = bam->GetCycleSize(cycle);
	bool resetIndex = false;
	if (core->HasFeature(GFFlags::IGNORE_BUTTON_FRAMES) && (cycleSize == 3 || cycleSize == 4)) {
		resetIndex = true;
	}
	if (resetIndex) {
		selectedIndex = 2;
		if (cycleSize == 4) disabledIndex = 3;
	}
	tspr = bam->GetFrame(selectedIndex, cycle);
	btn->SetImage(ButtonImage::Selected, std::move(tspr));
	tspr = bam->GetFrame(disabledIndex, cycle);
	btn->SetImage(ButtonImage::Disabled, std::move(tspr));

	return;
}

// GemRB specific control: progressbar
static void GetProgressbar(DataStream* str, Control*& ctrl, const Region& ctrlFrame)
{
	ResRef mosFile;
	ResRef mosFile2;
	ResRef bamFile;
	Point knobP;
	Point capP;
	ieWord knobStepsCount;
	ieWord cycle;

	str->ReadResRef(mosFile);
	str->ReadResRef(mosFile2);
	str->ReadResRef(bamFile);
	str->ReadWord(knobStepsCount);
	str->ReadWord(cycle);
	str->ReadPoint(knobP);
	str->ReadPoint(capP);
	Progressbar* pbar = new Progressbar(ctrlFrame, knobStepsCount);
	ctrl = pbar;
	pbar->SetSliderPos(knobP, capP);

	Holder<Sprite2D> img;
	Holder<Sprite2D> img2;
	Holder<Sprite2D> img3;
	if (!mosFile.IsEmpty()) {
		ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(mosFile);
		img = mos->GetSprite2D();
	}
	if (!mosFile2.IsEmpty()) {
		ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(mosFile2);
		img2 = mos->GetSprite2D();
	}

	if (knobStepsCount) {
		auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(bamFile, IE_BAM_CLASS_ID);
		if (af) {
			pbar->SetAnimation(af->GetCycle(cycle & 0xff));
		} else {
			Log(ERROR, "CHUImporter", "Couldn't create animationfactory for knob: {}", bamFile);
		}
	} else {
		ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(bamFile);
		img3 = mos->GetSprite2D();
	}
	pbar->SetBackground(std::move(img));
	pbar->SetImages(std::move(img2), std::move(img3));
}

static void GetSlider(DataStream* str, Control*& ctrl, const Region& ctrlFrame)
{
	ResRef mosFile;
	ResRef bamFile;
	Point knobPos;
	ieWord cycle;
	ieWord knob;
	ieWord grabbedKnob;
	ieWord knobStep;
	ieWord knobStepsCount;

	str->ReadResRef(mosFile);
	str->ReadResRef(bamFile);
	str->ReadWord(cycle);
	str->ReadWord(knob);
	str->ReadWord(grabbedKnob);
	str->ReadPoint(knobPos);
	str->ReadWord(knobStep);
	str->ReadWord(knobStepsCount);
	// ee documents 4 more words: ActiveBarTop, bottom, left, right

	Slider* sldr = new Slider(ctrlFrame, knobPos, knobStep, knobStepsCount);
	ctrl = sldr;
	ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(mosFile);
	Holder<Sprite2D> img = mos->GetSprite2D();
	sldr->SetImage(IE_GUI_SLIDER_BACKGROUND, img);

	auto bam = gamedata->GetFactoryResourceAs<const AnimationFactory>(bamFile, IE_BAM_CLASS_ID);
	if (bam) {
		img = bam->GetFrame(knob, 0);
		sldr->SetImage(IE_GUI_SLIDER_KNOB, img);
		img = bam->GetFrame(grabbedKnob, 0);
		sldr->SetImage(IE_GUI_SLIDER_GRABBEDKNOB, img);
	} else {
		sldr->SetState(IE_GUI_SLIDER_BACKGROUND);
	}
}

static void GetTextEdit(DataStream* str, Control*& ctrl, const Region& ctrlFrame)
{
	ResRef bgMos;
	ResRef editingMos;
	ResRef overMos;
	ResRef fontResRef;
	ResRef cursorResRef;
	Point pos;
	Point pos2;
	ieWord maxInput;
	ieWord curCycle;
	ieWord curFrame;
	ieVariable initial;

	str->ReadResRef(bgMos);
	str->ReadResRef(editingMos); // EditClientFocus
	str->ReadResRef(overMos); // EditClientNoFocus
	str->ReadResRef(cursorResRef);
	str->ReadWord(curCycle);
	str->ReadWord(curFrame);
	str->ReadPoint(pos); // "XEditClientOffset" and "YEditClientOffset" in the original
	// NOTE: I still don't know what to do with this point
	// Contrary to forum posts, it is definitely not a scrollbar ID
	// ee docs call them XEditCaretOffset and YEditCaretOffset
	// "pos" does affect the caret position in the originals, so we don't need an extra point
	str->ReadPoint(pos2);
	str->ReadResRef(fontResRef);
	//this field is still unknown or unused, labeled SequenceText
	str->Seek(2, GEM_CURRENT_POS);
	// This is really a text field, but apparently the original engine
	// always writes it over, and never uses it (labeled DefaultString)
	str->ReadVariable(initial);
	str->ReadWord(maxInput);
	// word: caseformat: 0 normal, 1 upper, 2 lower (Allowed case in NI) - not working in the original
	// word: typeformat, unknown
	auto fnt = core->GetFont(fontResRef);

	auto bam = gamedata->GetFactoryResourceAs<const AnimationFactory>(cursorResRef, IE_BAM_CLASS_ID);
	Holder<Sprite2D> cursor;
	if (bam) {
		cursor = bam->GetFrame(curCycle, curFrame);
	}

	TextEdit* te = new TextEdit(ctrlFrame, maxInput, pos);
	ctrl = te;
	te->SetFont(std::move(fnt));
	te->SetCursor(std::move(cursor));
	te->SetBackground(bgMos, TextEditBG::Normal);
	te->SetBackground(editingMos, TextEditBG::Editing);
	te->SetBackground(overMos, TextEditBG::Over);
	te->SetBackground(TextEditBG::Normal);
}

static void GetTextArea(DataStream* str, Control*& ctrl, const Region& ctrlFrame, Window*& win)
{
	ResRef fontResRef;
	ResRef initResRef;
	Color fore;
	Color init;
	Color back;
	ieWord scrollbarID;

	str->ReadResRef(fontResRef);
	str->ReadResRef(initResRef);
	auto textFont = core->GetFont(fontResRef);
	auto initialsFont = core->GetFont(initResRef);
	str->Read(&fore, 4);
	str->Read(&init, 4);
	str->Read(&back, 4);
	str->ReadWord(scrollbarID);

	fore.a = init.a = back.a = 0xff;

	TextArea* ta = new TextArea(ctrlFrame, std::move(textFont), std::move(initialsFont));
	ctrl = ta;
	ta->SetColor(fore, TextArea::COLOR_NORMAL);
	ta->SetColor(init, TextArea::COLOR_INITIALS);
	ta->SetColor(back, TextArea::COLOR_BACKGROUND);
	if (scrollbarID != 0xffff) {
		ScrollBar* sb = GetControl<ScrollBar>(scrollbarID, win);
		if (sb) {
			ta->SetScrollbar(sb);
		}
	}
}

static void GetLabel(DataStream* str, Control*& ctrl, const Region& ctrlFrame)
{
	ResRef fontResRef;
	ieStrRef textRef;
	ieWord alignment;
	Color textCol;
	Color bgCol;

	str->ReadStrRef(textRef);
	str->ReadResRef(fontResRef);
	str->Read(&textCol, 4);
	str->Read(&bgCol, 4);
	str->ReadWord(alignment);

	auto fnt = core->GetFont(fontResRef);
	textCol.a = bgCol.a = 0xff;
	String text = core->GetString(textRef);
	Label* lab = new Label(ctrlFrame, std::move(fnt), text);
	ctrl = lab;

	if (alignment & 1) {
		lab->SetFlags(Label::UseColor, BitOp::OR);
	}
	lab->SetColors(textCol, bgCol);

	unsigned char align = IE_FONT_ALIGN_CENTER;
	if ((alignment & 0x10) != 0) {
		align = IE_FONT_ALIGN_RIGHT;
	} else if ((alignment & 0x04) != 0) {
		// center, nothing to do
	} else if ((alignment & 0x08) != 0) {
		align = IE_FONT_ALIGN_LEFT;
	}

	if ((alignment & 0x20) != 0) {
		align |= IE_FONT_ALIGN_TOP;
	} else if ((alignment & 0x80) != 0) {
		align |= IE_FONT_ALIGN_BOTTOM;
	} else {
		align |= IE_FONT_ALIGN_MIDDLE;
	}
	lab->SetAlignment(align);
}

static void GetScrollbar(DataStream* str, Control*& ctrl, const Region& ctrlFrame, Window*& win, ieDword controlID)
{
	ResRef bamResRef;
	ieWord cycle;
	ieWord textareaID;
	ieWord imgIdx;
	str->ReadResRef(bamResRef);
	str->ReadWord(cycle);

	auto bam =gamedata->GetFactoryResourceAs<const AnimationFactory>(bamResRef, IE_BAM_CLASS_ID);
	if (!bam) {
		Log(ERROR, "CHUImporter", "Unable to create scrollbar, no BAM: {}", bamResRef);
		return;
	}
	Holder<Sprite2D> images[ScrollBar::IMAGE_COUNT];
	for (auto& image : images) {
		str->ReadWord(imgIdx);
		image = bam->GetFrame(imgIdx, cycle);
	}
	str->ReadWord(textareaID);

	ScrollBar* sb = new ScrollBar(ctrlFrame, images);
	if (textareaID == 0xffff) {
		// text areas produce their own scrollbars in GemRB
		ctrl = sb;
	} else {
		TextArea* ta = GetControl<TextArea>(textareaID, win);
		if (ta) {
			ta->SetScrollbar(sb);
		} else {
			ctrl = sb;
			// NOTE: we dont delete this, because there are at least a few instances
			// where the CHU has this assigned to a text area even tho there isnt one! (BG1 GUISTORE:RUMORS, PST ContainerWindow)
			// set them invisible instead, we will unhide them in the scripts that need them
			sb->SetVisible(false);
		}
		// we still allow GUIScripts to get ahold of it
		RegisterScriptableControl(sb, controlID);
	}
}

/** Returns the i-th window in the Previously Loaded Stream */
Window* CHUImporter::GetWindow(ScriptingId wid) const
{
	ieWord windowID;
	ieWord backGround;
	ieWord controlsCount;
	ieWord firstControl;
	ResRef mosFile;
	Region rgn;

	if (!str) {
		Log(ERROR, "CHUImporter", "No data stream to read from, skipping controls");
		return nullptr;
	}

	bool found = false;
	for (ieDword c = 0; c < WindowCount; c++) {
		str->Seek(WEOffset + 0x1c * c, GEM_STREAM_START);
		str->ReadWord(windowID);
		if (windowID == wid) {
			found = true;
			break;
		}
	}
	if (!found) {
		return nullptr;
	}

	str->Seek(2, GEM_CURRENT_POS); // windowID was a dword in bg2
	str->ReadRegion(rgn);
	str->ReadWord(backGround);
	str->ReadWord(controlsCount);
	str->ReadResRef(mosFile);
	str->ReadWord(firstControl);
	// also a word for Flags with only bit 0 relating to modal windows known ("don't dim the bg" in NI)

	Window* win = CreateWindow(windowID, rgn);
	Holder<Sprite2D> bg;
	if (backGround == 1) {
		ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(mosFile);
		if (mos) {
			bg = mos->GetSprite2D();
		}
	}
	win->SetBackground(std::move(bg));

	for (unsigned int i = 0; i < controlsCount; i++) {
		Control* ctrl = nullptr;
		str->Seek(CTOffset + (firstControl + i) * 8, GEM_STREAM_START);
		ieDword ctrlOffset;
		ieDword ctrlLength;
		ieDword controlID;
		Region ctrlFrame;
		ieByte controlType;
		ieByte tmp;

		str->ReadDword(ctrlOffset);
		str->ReadDword(ctrlLength);
		str->Seek(ctrlOffset, GEM_STREAM_START);
		str->ReadDword(controlID);
		str->ReadRegion(ctrlFrame);
		str->Read(&controlType, 1);
		str->Read(&tmp, 1); // in bg2 ControlType was a word
		switch (controlType) {
			case IE_GUI_BUTTON:
				GetButton(str, ctrl, ctrlFrame, !rgn.w);
				break;
			case IE_GUI_PROGRESSBAR:
				GetProgressbar(str, ctrl, ctrlFrame);
				break;
			case IE_GUI_SLIDER:
				GetSlider(str, ctrl, ctrlFrame);
				break;
			case IE_GUI_EDIT:
				GetTextEdit(str, ctrl, ctrlFrame);
				break;
			case IE_GUI_TEXTAREA:
				GetTextArea(str, ctrl, ctrlFrame, win);
				break;
			case IE_GUI_LABEL:
				GetLabel(str, ctrl, ctrlFrame);
				break;
			case IE_GUI_SCROLLBAR:
				GetScrollbar(str, ctrl, ctrlFrame, win, controlID);
				break;
			default:
				Log(ERROR, "CHUImporter", "Control Not Supported");
		}
		if (ctrl) {
			win->AddSubviewInFrontOfView(ctrl);
			RegisterScriptableControl(ctrl, controlID);
		}
	}
	return win;
}

/** Returns the number of available windows */
unsigned int CHUImporter::GetWindowsCount() const
{
	return WindowCount;
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool CHUImporter::LoadWindowPack(const ScriptingGroup_t& ref)
{
	if (ref == winPack) {
		return true; // already loaded
	}

	DataStream* stream = gamedata->GetResourceStream(ref, IE_CHU_CLASS_ID);
	if (stream == nullptr) {
		Log(ERROR, "CHUImporter", "Error: Cannot find {}.chu!", ref);
		return false;
	}
	if (!Open(stream)) {
		Log(ERROR, "CHUImporter", "Error: Cannot load {}.chu!", ref);
		return false;
	}

	winPack = ref;
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x23A7F6CA, "CHU File Importer")
PLUGIN_CLASS(IE_CHU_CLASS_ID, ImporterPlugin<CHUImporter>)
END_PLUGIN()
