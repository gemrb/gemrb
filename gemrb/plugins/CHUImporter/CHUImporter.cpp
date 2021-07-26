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
	str->Read( Signature, 8 );
	if (strncmp( Signature, "CHUIV1  ", 8 ) != 0) {
		Log(ERROR, "CHUImporter", "Not a Valid CHU File");
		return false;
	}
	str->ReadDword(WindowCount);
	str->ReadDword(CTOffset);
	str->ReadDword(WEOffset);
	return true;
}

/** Returns the i-th window in the Previously Loaded Stream */
Window* CHUImporter::GetWindow(ScriptingId wid) const
{
	ieWord WindowID, XPos, YPos, Width, Height, BackGround;
	ieWord ControlsCount, FirstControl;
	ResRef MosFile;

	if (!str) {
		Log(ERROR, "CHUImporter", "No data stream to read from, skipping controls");
		return NULL;
	}

	bool found = false;
	for (unsigned int c = 0; c < WindowCount; c++) {
		str->Seek( WEOffset + ( 0x1c * c ), GEM_STREAM_START );
		str->ReadWord(WindowID);
		if (WindowID == wid) {
			found = true;
			break;
		}
	}
	if (!found) {
		return NULL;
	}
	str->Seek( 2, GEM_CURRENT_POS );
	str->ReadWord(XPos);
	str->ReadWord(YPos);
	str->ReadWord(Width);
	str->ReadWord(Height);
	str->ReadWord(BackGround);
	str->ReadWord(ControlsCount);
	str->ReadResRef( MosFile );
	str->ReadWord(FirstControl);

	Window* win = CreateWindow(WindowID, Region(XPos, YPos, Width, Height));
	Holder<Sprite2D> bg;
	if (BackGround == 1) {
		ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(MosFile);
		if (mos) {
			bg = mos->GetSprite2D();
		}
	}
	win->SetBackground(bg);

	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		Log(ERROR, "CHUImporter", "No BAM Importer Available, skipping controls");
		return win;
	}
	for (unsigned int i = 0; i < ControlsCount; i++) {
		Control* ctrl = NULL;
		str->Seek( CTOffset + ( ( FirstControl + i ) * 8 ), GEM_STREAM_START );
		ieDword COffset, CLength, ControlID;
		Region ctrlFrame;
		ieWord tmp;
		ieByte ControlType, temp;
		str->ReadDword(COffset);
		str->ReadDword(CLength);
		str->Seek( COffset, GEM_STREAM_START );
		str->ReadDword(ControlID);
		str->ReadWord(tmp);
		ctrlFrame.x = tmp;
		str->ReadWord(tmp);
		ctrlFrame.y = tmp;
		str->ReadWord(tmp);
		ctrlFrame.w = tmp;
		str->ReadWord(tmp);
		ctrlFrame.h = tmp;
		str->Read( &ControlType, 1 );
		str->Read( &temp, 1 );
		switch (ControlType) {
			case IE_GUI_BUTTON:
			{
				//Button
				Button* btn = new Button(ctrlFrame);
				ctrl = btn;
				ResRef BAMFile;
				ieByte Cycle, tmp;
				ieDword Flags;
				ieByte UnpressedIndex, x1;
				ieByte PressedIndex, x2;
				ieByte SelectedIndex, y1;
				ieByte DisabledIndex, y2;
				str->ReadResRef( BAMFile );
				str->Read( &Cycle, 1 );
				str->Read( &tmp, 1 );
				Flags = ((ieDword) tmp)<<8;
				str->Read( &UnpressedIndex, 1 );
				str->Read( &x1, 1 );
				str->Read( &PressedIndex, 1 );
				str->Read( &x2, 1 );
				str->Read( &SelectedIndex, 1 );
				str->Read( &y1, 1 );
				str->Read( &DisabledIndex, 1 );
				str->Read( &y2, 1 );


				/** Justification comes from the .chu, other bits are set by script */
				if (!Width) {
					btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR);
				}
				if (core->HasFeature(GF_UPPER_BUTTON_TEXT)) {
					btn->SetFlags(IE_GUI_BUTTON_CAPS, OP_OR);
				}

				btn->SetFlags(Flags, OP_OR);
				if (Flags & IE_GUI_BUTTON_ANCHOR) {
					btn->SetAnchor(x1 | (x2<<8), y1 | (y2<<8));
				}

				if (BAMFile == "guictrl" && UnpressedIndex == 0) {
					break;
				}
				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID);
				if (!bam ) {
					Log(ERROR, "CHUImporter", "Cannot Load Button Images, skipping control");
					/* IceWind Dale 2 has fake BAM ResRefs for some Buttons,
					this will handle bad ResRefs */
					break;
				}
				/** Cycle is only a byte for buttons */
				Holder<Sprite2D> tspr = bam->GetFrame(UnpressedIndex, Cycle);

				btn->SetImage( BUTTON_IMAGE_UNPRESSED, tspr );
				tspr = bam->GetFrame( PressedIndex, Cycle );
				btn->SetImage( BUTTON_IMAGE_PRESSED, tspr );
				// work around several controls not setting all the indices
				AnimationFactory::index_t cycleSize = bam->GetCycleSize(Cycle);
				bool resetIndex = false;
				if (core->HasFeature(GF_IGNORE_BUTTON_FRAMES) && (cycleSize == 3 || cycleSize == 4)) {
						resetIndex = true;
				}
				if (resetIndex) {
						SelectedIndex = 2;
						if (cycleSize == 4) DisabledIndex = 3;
				}
				tspr = bam->GetFrame(SelectedIndex, Cycle);
				btn->SetImage( BUTTON_IMAGE_SELECTED, tspr );
				tspr = bam->GetFrame(DisabledIndex, Cycle);
				btn->SetImage( BUTTON_IMAGE_DISABLED, tspr );
			}
			break;

			case IE_GUI_PROGRESSBAR:
			{
				//GemRB specific, progressbar
				ResRef MOSFile;
				ResRef MOSFile2;
				ResRef BAMFile;
				Point knobP;
				Point capP;
				ieWord KnobStepsCount;
				ieWord Cycle;

				str->ReadResRef( MOSFile );
				str->ReadResRef( MOSFile2 );
				str->ReadResRef( BAMFile );
				str->ReadWord(KnobStepsCount);
				str->ReadWord(Cycle);
				str->ReadPoint(knobP);
				str->ReadPoint(capP);
				Progressbar* pbar = new Progressbar(ctrlFrame, KnobStepsCount);
				pbar->SetSliderPos(knobP, capP);

				Holder<Sprite2D> img;
				Holder<Sprite2D> img2;
				Holder<Sprite2D> img3;
				if (!MOSFile.IsEmpty()) {
					ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(MOSFile);
					img = mos->GetSprite2D();
				}
				if (!MOSFile2.IsEmpty()) {
					ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(MOSFile2);
					img2 = mos->GetSprite2D();
				}

				if( KnobStepsCount ) {
					/* getting the bam */
					AnimationFactory *af = (AnimationFactory *)
						gamedata->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID );
					/* Getting the Cycle of the bam */
					if (af) {
						pbar->SetAnimation(af->GetCycle( Cycle & 0xff ) );
					} else {
						Log(ERROR, "CHUImporter", "Couldn't create animationfactory for knob: %s", BAMFile.CString());
					}
				}
				else {
					ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(BAMFile);
					img3 = mos->GetSprite2D();
				}
				pbar->SetBackground(img);
				pbar->SetImages(img2, img3);
				ctrl = pbar;
			}
			break;
			case IE_GUI_SLIDER:
			{
				//Slider
				ResRef MOSFile;
				ResRef BAMFile;
				ieWord Cycle, Knob, GrabbedKnob;
				ieWord KnobXPos, KnobYPos, KnobStep, KnobStepsCount;
				str->ReadResRef( MOSFile );
				str->ReadResRef( BAMFile );
				str->ReadWord(Cycle);
				str->ReadWord(Knob);
				str->ReadWord(GrabbedKnob);
				str->ReadWord(KnobXPos);
				str->ReadWord(KnobYPos);
				str->ReadWord(KnobStep);
				str->ReadWord(KnobStepsCount);
				Slider* sldr = new Slider(ctrlFrame, Point(KnobXPos, KnobYPos), KnobStep, KnobStepsCount);
				ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(MOSFile);
				Holder<Sprite2D> img = mos->GetSprite2D();
				sldr->SetImage( IE_GUI_SLIDER_BACKGROUND, img);

				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID);
				if( bam ) {
					img = bam->GetFrame( Knob, 0 );
					sldr->SetImage( IE_GUI_SLIDER_KNOB, img );
					img = bam->GetFrame( GrabbedKnob, 0 );
					sldr->SetImage( IE_GUI_SLIDER_GRABBEDKNOB, img );
				}
				else {
					sldr->SetState(IE_GUI_SLIDER_BACKGROUND);
				}
				ctrl = sldr;
			}
			break;

			case IE_GUI_EDIT:
			{
				//Text Edit
				ResRef BGMos;
				ResRef FontResRef;
				ResRef CursorResRef;
				ieWord maxInput;
				ieWord CurCycle, CurFrame;
				ieWord PosX, PosY;
				ieWord Pos2X, Pos2Y;
				ieVariable Initial;

				str->ReadResRef( BGMos );
				//These are two more MOS resrefs, probably unused
				str->Seek( 16, GEM_CURRENT_POS );
				str->ReadResRef( CursorResRef );
				str->ReadWord(CurCycle);
				str->ReadWord(CurFrame);
				str->ReadWord(PosX);
				str->ReadWord(PosY);
				//FIXME: I still don't know what to do with this point
				//Contrary to forum posts, it is definitely not a scrollbar ID
				str->ReadWord(Pos2X);
				str->ReadWord(Pos2Y);
				str->ReadResRef( FontResRef );
				//this field is still unknown or unused
				str->Seek( 2, GEM_CURRENT_POS );
				//This is really a text field, but apparently the original engine
				//always writes it over, and never uses it
				str->Read( Initial, 32 );
				Initial[32]=0;
				str->ReadWord(maxInput);
				Font* fnt = core->GetFont( FontResRef );

				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource(CursorResRef, IE_BAM_CLASS_ID);
				Holder<Sprite2D> cursor;
				if (bam) {
					cursor = bam->GetFrame( CurCycle, CurFrame );
				}

				ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(BGMos);
				Holder<Sprite2D> img;
				if(mos) {
					img = mos->GetSprite2D();
				}

				TextEdit* te = new TextEdit( ctrlFrame, maxInput, Point(PosX, PosY) );
				te->SetFont( fnt );
				te->SetCursor( cursor );
				te->SetBackground( img );
				//The original engine always seems to ignore this textfield
				//te->SetText (Initial );
				ctrl = te;
			}
			break;

			case IE_GUI_TEXTAREA:
			{
				//Text Area
				ResRef FontResRef;
				ResRef InitResRef;
				Color fore, init, back;
				ieWord SBID;
				str->ReadResRef( FontResRef );
				str->ReadResRef( InitResRef );
				Font* fnt = core->GetFont( FontResRef );
				Font* ini = core->GetFont( InitResRef );
				str->Read( &fore, 4 );
				str->Read( &init, 4 );
				str->Read( &back, 4 );
				str->ReadWord(SBID);
				
				fore.a = init.a = back.a = 0xff;

				TextArea* ta = new TextArea(ctrlFrame, fnt, ini);
				ta->SetColor(fore, TextArea::COLOR_NORMAL);
				ta->SetColor(init, TextArea::COLOR_INITIALS);
				ta->SetColor(back, TextArea::COLOR_BACKGROUND);
				if (SBID != 0xffff) {
					ScrollBar* sb = GetControl<ScrollBar>(SBID, win);
					if (sb) {
						ta->SetScrollbar(sb);
					}
				}
				ctrl = ta;
			}
			break;

			case IE_GUI_LABEL:
			{
				//Label
				ResRef FontResRef;
				ieStrRef StrRef;
				ieWord alignment;
				str->ReadDword(StrRef);
				str->ReadResRef( FontResRef );
				Font* fnt = core->GetFont( FontResRef );

				Color textCol, bgCol;
				str->Read(&textCol, 4);
				str->Read(&bgCol, 4);
				
				textCol.a = bgCol.a = 0xff;

				str->ReadWord(alignment);
				String* str = core->GetString( StrRef );
				Label* lab = new Label(ctrlFrame, fnt, *str);
				delete str;

				if (alignment & 1) {
					lab->SetFlags(Label::UseColor, OP_OR);
				}
				lab->SetColors(textCol, bgCol);
				int align = IE_FONT_ALIGN_CENTER;
				if (( alignment & 0x10 ) != 0) {
					align = IE_FONT_ALIGN_RIGHT;
					goto endvertical;
				}
				if (( alignment & 0x04 ) != 0) {
					goto endvertical;
				}
				if (( alignment & 0x08 ) != 0) {
					align = IE_FONT_ALIGN_LEFT;
					goto endvertical;
				}
endvertical:
				if (( alignment & 0x20 ) != 0) {
					align |= IE_FONT_ALIGN_TOP;
					goto endalign;
				}
				if (( alignment & 0x80 ) != 0) {
					align |= IE_FONT_ALIGN_BOTTOM;
				} else {
					align |= IE_FONT_ALIGN_MIDDLE;
				}
endalign:
				lab->SetAlignment( align );
				ctrl = lab;
			}
			break;

			case IE_GUI_SCROLLBAR:
			{
				//ScrollBar
				ResRef BAMResRef;
				ieWord Cycle, TAID, imgIdx;
				str->ReadResRef( BAMResRef );
				str->ReadWord(Cycle);

				AnimationFactory* bam = ( AnimationFactory* )
				gamedata->GetFactoryResource(BAMResRef, IE_BAM_CLASS_ID);
				if (!bam) {
					Log(ERROR, "CHUImporter", "Unable to create scrollbar, no BAM: %s", BAMResRef.CString());
					break;
				}
				Holder<Sprite2D> images[ScrollBar::IMAGE_COUNT];
				for (auto& image : images) {
					str->ReadWord(imgIdx);
					image = bam->GetFrame(imgIdx, Cycle);
				}
				str->ReadWord(TAID);

				ScrollBar* sb = new ScrollBar(ctrlFrame, images);

				if (TAID == 0xffff) {
					// text areas produce their own scrollbars in GemRB
					ctrl = sb;
				} else {
					TextArea* ta = GetControl<TextArea>(TAID, win);
					if (ta) {
						ta->SetScrollbar(sb);
					} else {
						ctrl = sb;
						// NOTE: we dont delete this, becuase there are at least a few instances
						// where the CHU has this assigned to a text area even tho there isnt one! (BG1 GUISTORE:RUMORS, PST ContainerWindow)
						// set them invisible instead, we will unhide them in the scripts that need them
						sb->SetVisible(false);
					}
					// we still allow GUIScripts to get ahold of it
					RegisterScriptableControl(sb, ControlID);
				}
			}
			break;

			default:
				Log(ERROR, "CHUImporter", "Control Not Supported");
		}
		if (ctrl) {
			win->AddSubviewInFrontOfView( ctrl );
			RegisterScriptableControl(ctrl, ControlID);
		}
	}
	return win;
}
/** Returns the number of available windows */
unsigned int CHUImporter::GetWindowsCount()
{
	return WindowCount;
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool CHUImporter::LoadWindowPack(const ResRef& ref)
{
	if (ref == winPack) {
		return true; // already loaded
	}

	DataStream* stream = gamedata->GetResource( ref, IE_CHU_CLASS_ID );
	if (stream == NULL) {
		Log(ERROR, "CHUImporter", "Error: Cannot find %s.chu", ref.CString() );
		return false;
	}
	if (!Open(stream)) {
		Log(ERROR, "CHUImporter", "Error: Cannot Load %s.chu", ref.CString() );
		return false;
	}

	winPack = ref;
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x23A7F6CA, "CHU File Importer")
PLUGIN_CLASS(IE_CHU_CLASS_ID, ImporterPlugin<CHUImporter>)
END_PLUGIN()
