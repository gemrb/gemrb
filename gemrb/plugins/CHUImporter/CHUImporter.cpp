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
#include "win32def.h"

#include "AnimationFactory.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "GUI/Button.h"
#include "GUI/Label.h"
#include "GUI/Progressbar.h"
#include "GUI/ScrollBar.h"
#include "GUI/Slider.h"
#include "GUI/TextArea.h"
#include "GUI/TextEdit.h"
#include "GUI/Window.h"

using namespace GemRB;

CHUImporter::CHUImporter()
{
	str = NULL;
}

CHUImporter::~CHUImporter()
{
	delete str;
}

/** This function loads all available windows from the 'stream' parameter. */
bool CHUImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "CHUIV1  ", 8 ) != 0) {
		Log(ERROR, "CHUImporter", "Not a Valid CHU File");
		return false;
	}
	str->ReadDword( &WindowCount );
	str->ReadDword( &CTOffset );
	str->ReadDword( &WEOffset );
	return true;
}

/** Returns the i-th window in the Previously Loaded Stream */
Window* CHUImporter::GetWindow(unsigned int wid)
{
	ieWord WindowID, XPos, YPos, Width, Height, BackGround;
	ieWord ControlsCount, FirstControl;
	ieResRef MosFile;
	unsigned int i;

	bool found = false;
	for (unsigned int c = 0; c < WindowCount; c++) {
		str->Seek( WEOffset + ( 0x1c * c ), GEM_STREAM_START );
		str->ReadWord( &WindowID );
		if (WindowID == wid) {
			found = true;
			break;
		}
	}
	if (!found) {
		return NULL;
	}
	str->Seek( 2, GEM_CURRENT_POS );
	str->ReadWord( &XPos );
	str->ReadWord( &YPos );
	str->ReadWord( &Width );
	str->ReadWord( &Height );
	str->ReadWord( &BackGround );
	str->ReadWord( &ControlsCount );
	str->ReadResRef( MosFile );
	str->ReadWord( &FirstControl );

	Window* win = new Window( WindowID, XPos, YPos, Width, Height );
	if (BackGround == 1) {
		ResourceHolder<ImageMgr> mos(MosFile);
		if (mos != NULL) {
			win->SetBackGround( mos->GetSprite2D(), true );
		}
	}
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		Log(ERROR, "CHUImporter", "No BAM Importer Available, skipping controls");
		return win;
	}
	for (i = 0; i < ControlsCount; i++) {
		str->Seek( CTOffset + ( ( FirstControl + i ) * 8 ), GEM_STREAM_START );
		ieDword COffset, CLength, ControlID;
		ieWord XPos, YPos, Width, Height;
		ieByte ControlType, temp;
		str->ReadDword( &COffset );
		str->ReadDword( &CLength );
		str->Seek( COffset, GEM_STREAM_START );
		str->ReadDword( &ControlID );
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		str->ReadWord( &Width );
		str->ReadWord( &Height );
		str->Read( &ControlType, 1 );
		str->Read( &temp, 1 );
		switch (ControlType) {
			case IE_GUI_BUTTON:
			{
				//Button
				Button* btn = new Button( );
				btn->ControlID = ControlID;
				btn->XPos = XPos;
				btn->YPos = YPos;
				btn->Width = Width;
				btn->Height = Height;
				btn->ControlType = ControlType;
				ieResRef BAMFile;
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
				btn->Owner = win;
				/** Justification comes from the .chu, other bits are set by script */
				if (!Width) {
					btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BM_OR);
				}
				if (core->HasFeature(GF_UPPER_BUTTON_TEXT)) {
					btn->SetFlags(IE_GUI_BUTTON_CAPS, BM_OR);
				}

				btn->SetFlags( Flags, BM_OR );
				if (Flags & IE_GUI_BUTTON_ANCHOR) {
					btn->SetAnchor(x1 | (x2<<8), y1 | (y2<<8));
				}

				if (strnicmp( BAMFile, "guictrl\0", 8 ) == 0) {
					if (UnpressedIndex == 0) {
						//printMessage("CHUImporter", "Special Button Control, Skipping Image Loading\n",GREEN );
						win->AddControl( btn );
						break;
					}
				}
				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource( BAMFile,
							IE_BAM_CLASS_ID, IE_NORMAL );
				if (!bam ) {
					Log(ERROR, "CHUImporter", "Cannot Load Button Images, skipping control");
					/* IceWind Dale 2 has fake BAM ResRefs for some Buttons,
					this will handle bad ResRefs */
					win->AddControl( btn );
					break;
				}
				/** Cycle is only a byte for buttons */
				Sprite2D* tspr = bam->GetFrame( UnpressedIndex, (unsigned char) Cycle );
				btn->SetImage( IE_GUI_BUTTON_UNPRESSED, tspr );
				tspr = bam->GetFrame( PressedIndex, Cycle );
				btn->SetImage( IE_GUI_BUTTON_PRESSED, tspr );
				//ignorebuttonframes is a terrible hack
				if (core->HasFeature( GF_IGNORE_BUTTON_FRAMES) ) {
					if (bam->GetCycleSize(Cycle) == 4 )
						SelectedIndex=2;
				}
				tspr = bam->GetFrame( SelectedIndex, (unsigned char) Cycle );
				btn->SetImage( IE_GUI_BUTTON_SELECTED, tspr );
				if (core->HasFeature( GF_IGNORE_BUTTON_FRAMES) ) {
					if (bam->GetCycleSize( (unsigned char) Cycle) == 4 )
						DisabledIndex=3;
				}
				tspr = bam->GetFrame( DisabledIndex, (unsigned char) Cycle );
				btn->SetImage( IE_GUI_BUTTON_DISABLED, tspr );
				win->AddControl( btn );
			}
			break;

			case IE_GUI_PROGRESSBAR:
			{
				//GemRB specific, progressbar
				ieResRef MOSFile, MOSFile2;
				ieResRef BAMFile;
				ieWord KnobXPos, KnobYPos;
				ieWord CapXPos, CapYPos;
				ieWord KnobStepsCount;
				ieWord Cycle;

				str->ReadResRef( MOSFile );
				str->ReadResRef( MOSFile2 );
				str->ReadResRef( BAMFile );
				str->ReadWord( &KnobStepsCount );
				str->ReadWord( &Cycle );
				str->ReadWord( &KnobXPos );
				str->ReadWord( &KnobYPos );
				str->ReadWord( &CapXPos );
				str->ReadWord( &CapYPos );
				Progressbar* pbar = new Progressbar(KnobStepsCount, true );
				pbar->ControlID = ControlID;
				pbar->XPos = XPos;
				pbar->YPos = YPos;
				pbar->ControlType = ControlType;
				pbar->Width = Width;
				pbar->Height = Height;
				pbar->SetSliderPos( KnobXPos, KnobYPos, CapXPos, CapYPos );

				Sprite2D* img = NULL;
				Sprite2D* img2 = NULL;
				if ( MOSFile[0] ) {
					ResourceHolder<ImageMgr> mos(MOSFile);
					img = mos->GetSprite2D();
				}
				if ( MOSFile2[0] ) {
					ResourceHolder<ImageMgr> mos(MOSFile2);
					img2 = mos->GetSprite2D();
				}

				pbar->SetImage( img, img2 );
				if( KnobStepsCount ) {
					/* getting the bam */
					AnimationFactory *af = (AnimationFactory *)
						gamedata->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID );
					/* Getting the Cycle of the bam */
						pbar->SetAnimation(af->GetCycle( Cycle & 0xff ) );
				}
				else {
					ResourceHolder<ImageMgr> mos(BAMFile);
					Sprite2D* img3 = mos->GetSprite2D();
					pbar->SetBarCap( img3 );
				}
				win->AddControl( pbar );
			}
			break;
			case IE_GUI_SLIDER:
			{
				//Slider
				ieResRef MOSFile, BAMFile;
				ieWord Cycle, Knob, GrabbedKnob;
				ieWord KnobXPos, KnobYPos, KnobStep, KnobStepsCount;
				str->ReadResRef( MOSFile );
				str->ReadResRef( BAMFile );
				str->ReadWord( &Cycle );
				str->ReadWord( &Knob );
				str->ReadWord( &GrabbedKnob );
				str->ReadWord( &KnobXPos );
				str->ReadWord( &KnobYPos );
				str->ReadWord( &KnobStep );
				str->ReadWord( &KnobStepsCount );
				Slider* sldr = new Slider( KnobXPos, KnobYPos, KnobStep, KnobStepsCount, true );
				sldr->ControlID = ControlID;
				sldr->XPos = XPos;
				sldr->YPos = YPos;
				sldr->ControlType = ControlType;
				sldr->Width = Width;
				sldr->Height = Height;
				ResourceHolder<ImageMgr> mos(MOSFile);
				Sprite2D* img = mos->GetSprite2D();
				sldr->SetImage( IE_GUI_SLIDER_BACKGROUND, img);

				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource( BAMFile,
							IE_BAM_CLASS_ID, IE_NORMAL );
				if( bam ) {
					img = bam->GetFrame( Knob, 0 );
					sldr->SetImage( IE_GUI_SLIDER_KNOB, img );
					img = bam->GetFrame( GrabbedKnob, 0 );
					sldr->SetImage( IE_GUI_SLIDER_GRABBEDKNOB, img );
				}
				else {
					sldr->SetState(IE_GUI_SLIDER_BACKGROUND);
				}
				win->AddControl( sldr );
			}
			break;

			case IE_GUI_EDIT:
			{
				//Text Edit
				ieResRef BGMos;
				ieResRef FontResRef, CursorResRef;
				ieWord maxInput;
				ieWord CurCycle, CurFrame;
				ieWord PosX, PosY;
				ieWord Pos2X, Pos2Y;
				ieVariable Initial;

				str->ReadResRef( BGMos );
				//These are two more MOS resrefs, probably unused
				str->Seek( 16, GEM_CURRENT_POS );
				str->ReadResRef( CursorResRef );
				str->ReadWord( &CurCycle );
				str->ReadWord( &CurFrame );
				str->ReadWord( &PosX );
				str->ReadWord( &PosY );
				//FIXME: I still don't know what to do with this point
				//Contrary to forum posts, it is definitely not a scrollbar ID
				str->ReadWord( &Pos2X );
				str->ReadWord( &Pos2Y );
				str->ReadResRef( FontResRef );
				//this field is still unknown or unused
				str->Seek( 2, GEM_CURRENT_POS );
				//This is really a text field, but apparently the original engine
				//always writes it over, and never uses it
				str->Read( Initial, 32 );
				Initial[32]=0;
				str->ReadWord( &maxInput );
				Font* fnt = core->GetFont( FontResRef );

				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource( CursorResRef,
							IE_BAM_CLASS_ID,
							IE_NORMAL );
				Sprite2D *cursor = NULL;
				if (bam) {
					cursor = bam->GetFrame( CurCycle, CurFrame );
				}

				ResourceHolder<ImageMgr> mos(BGMos);
				Sprite2D *img = NULL;
				if(mos) {
					img = mos->GetSprite2D();
				}

				TextEdit* te = new TextEdit( maxInput, PosX, PosY );
				te->ControlID = ControlID;
				te->XPos = XPos;
				te->YPos = YPos;
				te->Width = Width;
				te->Height = Height;
				te->ControlType = ControlType;
				te->SetFont( fnt );
				te->SetCursor( cursor );
				te->SetBackGround( img );
				//The original engine always seems to ignore this textfield
				//te->SetText (Initial );
				win->AddControl( te );
			}
			break;

			case IE_GUI_TEXTAREA:
			{
				//Text Area
				ieResRef FontResRef, InitResRef;
				Color fore, init, back;
				ieWord SBID;
				str->ReadResRef( FontResRef );
				str->ReadResRef( InitResRef );
				Font* fnt = core->GetFont( FontResRef );
				Font* ini = core->GetFont( InitResRef );
				str->Read( &fore, 4 );
				str->Read( &init, 4 );
				str->Read( &back, 4 );
				str->ReadWord( &SBID );
				TextArea* ta = new TextArea( fore, init, back );
				ta->ControlID = ControlID;
				ta->XPos = XPos;
				ta->YPos = YPos;
				ta->Width = Width;
				ta->Height = Height;
				ta->ControlType = ControlType;
				ta->SetFonts( ini, fnt );
				win->AddControl( ta );
				if (SBID != 0xffff)
					win->Link( SBID, ( unsigned short ) ControlID );
			}
			break;

			case IE_GUI_LABEL:
			{
				//Label
				ieResRef FontResRef;
				ieStrRef StrRef;
				RevColor fore, back;
				ieWord alignment;
				str->ReadDword( &StrRef );
				str->ReadResRef( FontResRef );
				Font* fnt = core->GetFont( FontResRef );
				str->Read( &fore, 4 );
				str->Read( &back, 4 );
				str->ReadWord( &alignment );
				Label* lab = new Label( fnt );
				lab->ControlID = ControlID;
				lab->XPos = XPos;
				lab->YPos = YPos;
				lab->Width = Width;
				lab->Height = Height;
				lab->ControlType = ControlType;
				char* str = core->GetString( StrRef );
				lab->SetText( str );
				core->FreeString( str );
				if (alignment & 1) {
					lab->useRGB = true;
					Color f, b;
					f.r = fore.b;
					f.g = fore.g;
					f.b = fore.r;
					f.a = 0;
					b.r = back.b;
					b.g = back.g;
					b.b = back.r;
					b.a = 0;
					lab->SetColor( f, b );
				}
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
				win->AddControl( lab );
			}
			break;

			case IE_GUI_SCROLLBAR:
			{
				//ScrollBar
				ieResRef BAMResRef;
				ieWord Cycle, Trough, Slider, TAID;
				ieWord UpUnPressed, UpPressed;
				ieWord DownUnPressed, DownPressed;

				str->ReadResRef( BAMResRef );
				str->ReadWord( &Cycle );
				str->ReadWord( &UpUnPressed );
				str->ReadWord( &UpPressed );
				str->ReadWord( &DownUnPressed );
				str->ReadWord( &DownPressed );
				str->ReadWord( &Trough );
				str->ReadWord( &Slider );
				str->ReadWord( &TAID );
				ScrollBar* sbar = new ScrollBar();
				sbar->ControlID = ControlID;
				sbar->XPos = XPos;
				sbar->YPos = YPos;
				sbar->Width = Width;
				sbar->Height = Height;
				sbar->ControlType = ControlType;

				AnimationFactory* bam = ( AnimationFactory* )
					gamedata->GetFactoryResource( BAMResRef,
							IE_BAM_CLASS_ID, IE_NORMAL );
				if (bam) {
					sbar->SetImage( IE_GUI_SCROLLBAR_UP_UNPRESSED,
									bam->GetFrame( UpUnPressed, Cycle ) );
					sbar->SetImage( IE_GUI_SCROLLBAR_UP_PRESSED,
									bam->GetFrame( UpPressed, Cycle ) );
					sbar->SetImage( IE_GUI_SCROLLBAR_DOWN_UNPRESSED,
									bam->GetFrame( DownUnPressed, Cycle ) );
					sbar->SetImage( IE_GUI_SCROLLBAR_DOWN_PRESSED,
									bam->GetFrame( DownPressed, Cycle ) );
					sbar->SetImage( IE_GUI_SCROLLBAR_TROUGH,
									bam->GetFrame( Trough, Cycle ) );
					sbar->SetImage( IE_GUI_SCROLLBAR_SLIDER,
									bam->GetFrame( Slider, Cycle ) );
				}
				win->AddControl( sbar );
				if (TAID != 0xffff)
					win->Link( ( unsigned short ) ControlID, TAID );
			}
			break;

			default:
				Log(ERROR, "CHUImporter", "Control Not Supported");
		}
	}
	return win;
}
/** Returns the number of available windows */
unsigned int CHUImporter::GetWindowsCount()
{
	return WindowCount;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x23A7F6CA, "CHU File Importer")
PLUGIN_CLASS(IE_CHU_CLASS_ID, CHUImporter)
END_PLUGIN()
