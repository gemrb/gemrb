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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "TextArea.h"
#include "Interface.h"
#include "Video.h"
#include "Palette.h"
#include "Variables.h"
#include "GameControl.h"
#include "Audio.h"
#include "Actor.h"
#include "ResourceMgr.h"  //for loading bmp image

#include <stdio.h>
#include <stdlib.h>

TextArea::TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor)
{
	keeplines = 100;
	rows = 0;
	startrow = 0;
	minrow = 0;
	seltext = -1;
	Value = 0xffffffff;
	ResetEventHandler( TextAreaOnChange );
	ResetEventHandler( TextAreaOutOfText );
	PortraitResRef[0]=0;
	palette = core->CreatePalette( hitextcolor, lowtextcolor );
	initpalette = core->CreatePalette( initcolor, lowtextcolor );
	Color tmp = {
		hitextcolor.b, hitextcolor.g, hitextcolor.r, 0
	};
	selected = core->CreatePalette( tmp, lowtextcolor );
	tmp.r = 255;
	tmp.g = 152;
	tmp.b = 102;
	lineselpal = core->CreatePalette( tmp, lowtextcolor );
	InternalFlags = 1;
	//Drop Capitals means initials on!
	core->GetDictionary()->Lookup("Drop Capitals", InternalFlags);
	if (InternalFlags) {
		InternalFlags = TA_INITIALS;
	}
}

TextArea::~TextArea(void)
{
	core->FreePalette( palette );
	core->FreePalette( initpalette );
	core->FreePalette( selected );
	core->FreePalette( lineselpal );
	for (size_t i = 0; i < lines.size(); i++) {
		free( lines[i] );
	}
}

void TextArea::RefreshSprite(const char *portrait)
{
	if (AnimPicture) {
		if (!strnicmp(PortraitResRef, portrait, 8) ) {
			return;
		}
		SetAnimPicture(NULL);
	}
	strnlwrcpy(PortraitResRef, portrait, 8);
	if (!strnicmp(PortraitResRef, "none", 8) ) {
		return;
	}
	DataStream* str = core->GetResourceMgr()->GetResource( PortraitResRef, IE_BMP_CLASS_ID );
	if (str==NULL) {
		return;
	}
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return;
	}

	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return;
	}

	SetAnimPicture ( im->GetImage() );
	core->FreeInterface( im );
}

void TextArea::Draw(unsigned short x, unsigned short y)
{
	/** Don't come back recursively */
	if (InternalFlags&TA_BITEMYTAIL) {
		return;
	}
	int tx=x+XPos;
	int ty=y+YPos;
	Region clip( tx, ty, Width, Height );
	Video *video = core->GetVideoDriver();

	if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
		if (AnimPicture) {
			video->BlitSprite(AnimPicture, tx,ty, true, &clip);
			clip.x+=AnimPicture->Width;
			clip.w-=AnimPicture->Width;
		}
	}

	//this might look better in GlobalTimer
	//or you might want to change the animated button to work like this
	if (Flags &IE_GUI_TEXTAREA_SMOOTHSCROLL)
	{
		unsigned long thisTime;

		GetTime( thisTime);
		if (thisTime>starttime) {
			starttime = thisTime+ticks;
			smooth--;
			while (smooth<=0) {
				smooth+=ftext->maxHeight;
				if (startrow<rows) {
					startrow++;
				}
			}

			/** Forcing redraw of whole screen before drawing text*/
			Owner->Invalidate();
			InternalFlags |= TA_BITEMYTAIL;
			Owner->DrawWindow();
			InternalFlags &= ~TA_BITEMYTAIL;
		}
	}

	if (!Changed && !(Owner->Flags&WF_FLOAT) ) {
		return;
	}
	Changed = false;

	if (XPos == 65535) {
		return;
	}
	size_t linesize = lines.size();
	if (linesize == 0) {
		return;
	}

	//smooth vertical scrolling up
	if (Flags & IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		clip.y+=smooth;
		clip.h-=smooth;
	}

	//if textarea is 'selectable' it actually means, it is a listbox
	//in this case the selected value equals the line number
	//if it is 'not selectable' it can still have selectable lines
	//but then it is like the dialog window in the main game screen:
	//the selected value is encoded into the line
	if (!(Flags & IE_GUI_TEXTAREA_SELECTABLE) ) {
		char* Buffer = ( char* ) malloc( 1 );
		Buffer[0] = 0;
		int len = 0;
		int lastlen = 0;
		for (size_t i = 0; i < linesize; i++) {
			if (strnicmp( "[s=", lines[i], 3 ) == 0) {
				int tlen;
				unsigned long idx, acolor, bcolor;
				char* rest;
				idx = strtoul( lines[i] + 3, &rest, 0 );
				if (*rest != ',')
					goto notmatched;
				acolor = strtoul( rest + 1, &rest, 16 );
				if (*rest != ',')
					goto notmatched;
				bcolor = strtoul( rest + 1, &rest, 16 );
				if (*rest != ']')
					goto notmatched;
				tlen = (int)(strstr( rest + 1, "[/s]" ) - rest - 1);
				if (tlen < 0)
					goto notmatched;
				len += tlen + 23;
				Buffer = ( char * ) realloc( Buffer, len + 2 );
				if (seltext == (int) i) {
					sprintf( Buffer + lastlen, "[color=%6.6lX]%.*s[/color]",
						acolor, tlen, rest + 1 );
				} else {
					sprintf( Buffer + lastlen, "[color=%6.6lX]%.*s[/color]",
						bcolor, tlen, rest + 1 );
				}
			} else {
				notmatched:
				len += ( int ) strlen( lines[i] ) + 1;
				Buffer = ( char * ) realloc( Buffer, len + 2 );
				memcpy( &Buffer[lastlen], lines[i], len - lastlen );
			}
			lastlen = len;
			if (i != linesize - 1) {
				Buffer[lastlen - 1] = '\n';
				Buffer[lastlen] = 0;
			}
		}
		video->SetClipRect( &clip );
		ftext->PrintFromLine( startrow, clip,
			( unsigned char * ) Buffer, palette,
			IE_FONT_ALIGN_LEFT, finit, NULL );
		free( Buffer );
		video->SetClipRect( NULL );
		//streaming text
		if (linesize>50) {
			//the buffer is filled enough
			return;
		}
		if (core->GetAudioDrv()->IsSpeaking() ) {
			//the narrator is still talking
			return;
		}
		if (RunEventHandler( TextAreaOutOfText )) {
			return;
		}
		if (linesize==lines.size()) {
			ResetEventHandler( TextAreaOutOfText );
			return;
		}
		AppendText("\n",-1);
		return;
	}
	// normal scrolling textarea
	int rc = 0;
	int sr = startrow;
	unsigned int i;
	int yl = 0;
	for (i = 0; i < linesize; i++) {
		if (rc + lrows[i] <= sr) {
			rc += lrows[i];
			continue;
		}
		sr -= rc;
		Palette* pal = NULL;
		if (seltext == (int) i)
			pal = selected;
		else if (Value == i)
			pal = lineselpal;
		else
			pal = palette;
		ftext->PrintFromLine( sr, clip,
			( unsigned char * ) lines[i], pal,
			IE_FONT_ALIGN_LEFT, finit, NULL );
		yl = lrows[i] - sr;
		break;
	}
	for (i++; i < linesize; i++) {
		Palette* pal = NULL;
		if (seltext == (int) i)
			pal = selected;
		else if (Value == i)
			pal = lineselpal;
		else
			pal = palette;
		clip.y+=ftext->size[1].h;
		clip.h-=ftext->size[1].h;
		ftext->Print( clip, ( unsigned char * ) lines[i], pal,
			IE_FONT_ALIGN_LEFT, true );

		yl += lrows[i];
	}
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Text Area Control. */
int TextArea::SetScrollBar(Control* ptr)
{
	int ret = Control::SetScrollBar(ptr);
	CalcRowCount();
	return ret;
}

/** Sets the Actual Text */
int TextArea::SetText(const char* text, int pos)
{
	if (pos==0) {
		if (!text[0]) {
			lines.clear();
			lrows.clear();
		}

		if (lines.size() == 0) {
			pos = -1;
		}
	}
	if (pos >= ( int ) lines.size()) {
		return -1;
	}
	int newlen = ( int ) strlen( text );

	if (pos == -1) {
		char* str = ( char* ) malloc( newlen + 1 );
		memcpy( str, text, newlen + 1 );
		lines.push_back( str );
		lrows.push_back( 0 );
	} else {
		lines[pos] = ( char * ) realloc( lines[pos], newlen + 1 );
		memcpy( lines[pos], text, newlen + 1 );
	}
	UpdateControls();
	return 0;
}

void TextArea::SetMinRow(bool enable)
{
	if (enable) {
		minrow = (int) lines.size();
	} else {
		minrow = 0;
	}
	Changed = true;
}

//drop lines scrolled out at the top.
//keeplines is the number of lines that should still be
//preserved (for scrollback history)
void TextArea::DiscardLines()
{
	if (rows<=keeplines) {
		return;
	}
	int drop = rows-keeplines;
	PopLines(drop, true);
}

static const char inserted_crap[]="[/color][color=ffffff]";
#define CRAPLENGTH sizeof(inserted_crap)-1

/** Appends a String to the current Text */
int TextArea::AppendText(const char* text, int pos)
{
	int ret = 0;
	if (pos >= ( int ) lines.size()) {
		return -1;
	}
	int newlen = ( int ) strlen( text );

	if (pos == -1) {
		const char *note = strstr(text,"\r\n\r\nNOTE:");
		char *str;
		if (NULL == note) {
			str = ( char* ) malloc( newlen +1 );
			memcpy(str,text, newlen+1);
		}
		else {
			unsigned int notepos = (unsigned int) (note - text);
			str = ( char* ) malloc( newlen + CRAPLENGTH+1 );
			memcpy(str,text,notepos);
			memcpy(str+notepos,inserted_crap,CRAPLENGTH);
			memcpy(str+notepos+CRAPLENGTH, text+notepos, newlen-notepos+1);
		}
		lines.push_back( str );
		lrows.push_back( 0 );
		ret =(int) (lines.size() - 1);
	} else {
		int mylen = ( int ) strlen( lines[pos] );

		lines[pos] = ( char * ) realloc( lines[pos], mylen + newlen + 1 );
		memcpy( lines[pos]+mylen, text, newlen + 1 );
		ret = pos;
	}

	//if the textarea is not a listbox, then discard scrolled out
	//lines
	if (Flags&IE_GUI_TEXTAREA_HISTORY) {
		DiscardLines();
	}

	UpdateControls();
	return ret;
}

/** Deletes last or first `count' lines */
/** Probably not too optimal for many lines, but it isn't used */
/** for many lines */
void TextArea::PopLines(unsigned int count, bool top)
{
	if (count > lines.size()) {
		count = (unsigned int) lines.size();
	}

	while (count > 0 ) {
		if (top) {
			int tmp = lrows.front();
			if (minrow || (startrow<tmp) )
				break;
			startrow -= tmp;
			free(lines.front() );
			lines.erase(lines.begin());
			lrows.erase(lrows.begin());
		} else {
			free(lines.back() );
			lines.pop_back();
			lrows.pop_back();
		}
		count--;
	}
	UpdateControls();
}

void TextArea::UpdateControls()
{
	int pos;

	CalcRowCount();
	Changed = true;
	if (sb) {
		ScrollBar* bar = ( ScrollBar* ) sb;
		if (Flags & IE_GUI_TEXTAREA_AUTOSCROLL)
			pos = rows - ( ( Height - 5 ) / ftext->maxHeight );
		else
			pos = 0;
		if (pos < 0)
			pos = 0;
		bar->SetPos( pos );
	} else {
		if (Flags & IE_GUI_TEXTAREA_AUTOSCROLL) {
			pos = rows - ( ( Height - 5 ) / ftext->maxHeight );
			SetRow(pos);
		}
	}
	core->RedrawAll();
}

/** Sets the Fonts */
void TextArea::SetFonts(Font* init, Font* text)
{
	finit = init;
	ftext = text;
	Changed = true;
}

/** Key Press Event */
void TextArea::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	//Selectable=false for dialogs, rather unintuitive, but fact
	if ((Flags & IE_GUI_TEXTAREA_SELECTABLE) || ( Key < '1' ) || ( Key > '9' ))
		return;
	GameControl *gc = core->GetGameControl();
	if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
		Changed = true;
		seltext=minrow-1;
		if ((unsigned int) seltext>=lines.size()) {
			return;
		}
		for(int i=0;i<Key-'0';i++) {
			do {
				seltext++;
				if ((unsigned int) seltext>=lines.size()) {
					return;
				}
			}
			while (strnicmp( lines[seltext], "[s=", 3 ) != 0 );
		}
		int idx=-1;
		sscanf( lines[seltext], "[s=%d,", &idx);
		if (idx==-1) {
			//this kills this object, don't use any more data!
			gc->EndDialog();
			return;
		}
		gc->DialogChoose( idx );
	}
}
/** Special Key Press */
void TextArea::OnSpecialKeyPress(unsigned char /*Key*/)
{
}

/** Returns Row count */
int TextArea::GetRowCount()
{
	return ( int ) lines.size();
}

int TextArea::GetVisibleRowCount()
{
	return (Height-5) / ftext->maxHeight;
}

/** Returns top index */
int TextArea::GetTopIndex()
{
	return startrow;
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row < rows) {
		startrow = row;
	}
	Changed = true;
}

void TextArea::CalcRowCount()
{
	int w = Width;

	if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
		const char *portrait = NULL;
		Actor *actor = NULL;
		GameControl *gc = core->GetGameControl();
		if (gc) {
			actor = gc->GetTarget();
		}
		if (actor) {
			portrait = actor->GetPortrait(1);
		}
		if (portrait) {
			RefreshSprite(portrait);
		}
		if (AnimPicture) {
			w-=AnimPicture->Width;
		}
	}

	rows = 0;
	if (lines.size() != 0) {
		for (size_t i = 0; i < lines.size(); i++) {
			rows++;
			int tr = 0;
			int len = ( int ) strlen( lines[i] );
			char* tmp = ( char* ) malloc( len + 1 );
			memcpy( tmp, lines[i], len + 1 );
			ftext->SetupString( tmp, w );
			for (int p = 0; p <= len; p++) {
				if (( ( unsigned char ) tmp[p] ) == '[') {
					p++;
					//char tag[256];
					int k = 0;
					for (k = 0; k < 256; k++) {
						if (tmp[p] == ']') {
							//tag[k] = 0;
							break;
						}
						p++;
						//tag[k] = tmp[p++];
					}

					continue;
				}
				if (tmp[p] == 0) {
					if (p != len)
						rows++;
					tr++;
				}
			}
			lrows[i] = tr;
			free( tmp );
		}
	}
	if (!sb) {
		return;
	}
	ScrollBar* bar = ( ScrollBar* ) sb;
	bar->SetMax( (ieWord) rows );
}
/** Mouse Over Event */
void TextArea::OnMouseOver(unsigned short /*x*/, unsigned short y)
{
	int height = ftext->maxHeight; //size[1].h;
	int r = y / height;
	int row = 0;

	for (size_t i = 0; i < lines.size(); i++) {
		row += lrows[i];
		if (r < ( row - startrow )) {
			if (seltext != (int) i)
				core->RedrawAll();
			seltext = ( int ) i;
			//printf("CtrlId = 0x%08lx, seltext = %d, rows = %d, row = %d, r = %d\n", ControlID, i, rows, row, r);
			return;
		}
	}
	if (seltext != -1) {
		core->RedrawAll();
	}
	seltext = -1;
	//printf("CtrlId = 0x%08lx, seltext = %d, rows %d, row %d, r = %d\n", ControlID, seltext, rows, row, r);
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char /*Button*/, unsigned short /*Mod*/)
{
	if (( x <= Width ) && ( y <= ( Height - 5 ) ) && ( seltext != -1 )) {
		Value = (unsigned int) seltext;
		if (strnicmp( lines[seltext], "[s=", 3 ) == 0) {
			if (minrow > seltext)
				return;
			int idx;
			sscanf( lines[seltext], "[s=%d,", &idx );
			GameControl* gc = core->GetGameControl();
			if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
				if (idx==-1) {
					//this kills this object, don't use any more data!
					gc->EndDialog();
					return;
				}
				gc->DialogChoose( idx );
				return;
			}
		}
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}
	RunEventHandler( TextAreaOnChange );
}

/** Copies the current TextArea content to another TextArea control */
void TextArea::CopyTo(TextArea* ta)
{
	ta->Clear();
	for (size_t i = 0; i < lines.size(); i++) {
		ta->SetText( lines[i], -1 );
	}
}

void TextArea::RedrawTextArea(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	Value = Sum;
	Changed = true;
}

const char* TextArea::QueryText()
{
	if ( Value<lines.size() ) {
		return ( const char * ) lines[Value];
	}
	return ( const char *) "";
}

bool TextArea::SetEvent(int eventType, const char *handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		SetEventHandler( TextAreaOnChange, handler );
		break;
	case IE_GUI_TEXTAREA_OUT_OF_TEXT:
		SetEventHandler( TextAreaOutOfText, handler );
		break;
	default:
		return false;
	}

	return true;
}

void TextArea::PadMinRow()
{
	int row = 0;
	int i=(int) (lines.size()-1);
	//minrow -1 ->gap
	//minrow -2 ->npc text
	while (i>=minrow-2 && i>=0) {
		row+=lrows[i];
		i--;
	}
	row = GetVisibleRowCount()-row;
	while (row>0) {
		AppendText("",-1);
		row--;
	}
}

void TextArea::SetPreservedRow(int arg)
{
	keeplines=arg;
	Flags |= IE_GUI_TEXTAREA_HISTORY;
}

void TextArea::Clear()
{
	for (size_t i = 0; i < lines.size(); i++) {
		free( lines[i] );
	}
	lines.clear();
	lrows.clear();
	rows = 0;
}

//setting up the textarea for smooth scrolling, the first
//TEXTAREA_OUTOFTEXT callback is called automatically
void TextArea::SetupScroll(unsigned long tck)
{
	SetPreservedRow(0);
	smooth = ftext->maxHeight;
	startrow = 0;
	ticks = tck;
	//clearing the textarea
	Clear();
	unsigned int i = (unsigned int) (Height/smooth);
	while (i--) {
		char *str = (char *) malloc(1);
		str[0]=0;
		lines.push_back(str);
		lrows.push_back(0);
	}
	i = (unsigned int) lines.size();
	Flags |= IE_GUI_TEXTAREA_SMOOTHSCROLL;
	GetTime( starttime );
	if (RunEventHandler( TextAreaOutOfText )) {
		//event handler destructed this object?
		return;
	}
	if (i==lines.size()) {
		ResetEventHandler( TextAreaOutOfText );
		return;
	}
	//recalculates rows
	AppendText("\n",-1);
}

void TextArea::OnMouseDown(unsigned short /*x*/, unsigned short /*y*/,
	unsigned char Button, unsigned short /*Mod*/)
{

	if (sb) {
		ScrollBar* scrlbr = (ScrollBar*)sb;
		switch(Button) {
		case GEM_MB_SCRLUP:
			scrlbr->ScrollUp();
			core->RedrawAll();
			break;
		case GEM_MB_SCRLDOWN:
			scrlbr->ScrollDown();
			core->RedrawAll();
			break;
		}
	}
}
