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

#include "GUI/TextArea.h"

#include "GUI/GameControl.h"

#include "win32def.h"

#include "Audio.h"
#include "DialogHandler.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Palette.h"
#include "Variables.h"
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"
#include "Scriptable/Actor.h"

#include <cstdio>
#include <cstdlib>

namespace GemRB {

TextArea::TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor)
{
	keeplines = 100;
	rows = 0;
	TextYPos = 0;
	startrow = 0;
	minrow = 0;
	Cursor = NULL;
	CurPos = 0;
	CurLine = 0;
	seltext = -1;
	Value = 0xffffffff;
	ResetEventHandler( TextAreaOnChange );
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
	InternalFlags = TA_INITIALS;
	//Drop Capitals means initials on!
	core->GetDictionary()->Lookup("Drop Capitals", InternalFlags);
	if (InternalFlags) {
		InternalFlags = TA_INITIALS;
	}
}

TextArea::~TextArea(void)
{
	gamedata->FreePalette( palette );
	gamedata->FreePalette( initpalette );
	gamedata->FreePalette( selected );
	gamedata->FreePalette( lineselpal );
	core->GetVideoDriver()->FreeSprite( Cursor );
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
	ResourceHolder<ImageMgr> im(PortraitResRef, true);
	if (im == NULL) {
		return;
	}

	SetAnimPicture ( im->GetSprite2D() );
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

		thisTime = GetTickCount();
		if (thisTime>starttime) {
			starttime = thisTime+ticks;

			TextYPos++;// can't use ScrollToY
			if (TextYPos % ftext->maxHeight == 0) SetRow(startrow + 1);

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

	//if textarea is 'selectable' it actually means, it is a listbox
	//in this case the selected value equals the line number
	//if it is 'not selectable' it can still have selectable lines
	//but then it is like the dialog window in the main game screen:
	//the selected value is encoded into the line
	if (!(Flags & IE_GUI_TEXTAREA_SELECTABLE) ) {
		char* Buffer = (char *) malloc( 1 );
		Buffer[0] = 0;
		int len = 0;
		int lastlen = 0;
		for (size_t i = 0; i < linesize; i++) {
			if (strnicmp( "[s=", lines[i], 3 ) == 0) {
				int tlen;
				unsigned long acolor, bcolor;
				char* rest = strchr( lines[i] + 3, ',' );
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
				Buffer = (char *) realloc( Buffer, len + 2 );
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
				Buffer = (char *) realloc( Buffer, len + 2 );
				memcpy( &Buffer[lastlen], lines[i], len - lastlen );
			}
			lastlen = len;
			if (i != linesize - 1) {
				Buffer[lastlen - 1] = '\n';
				Buffer[lastlen] = 0;
			}
		}
		video->SetClipRect( &clip );

		int pos;

		if (startrow==CurLine) {
			pos = CurPos;
		} else {
			pos = -1;
		}

		/* lets fake scrolling the text by simply offsetting the textClip by between 0 and maxHeight pixels.
			don't forget to increase the clipping height by the same amount */
		short LineOffset = (short)(TextYPos % ftext->maxHeight);
		Region textClip(clip.x, clip.y - LineOffset, clip.w, clip.h + LineOffset);


		ftext->PrintFromLine( startrow, textClip,
							 ( unsigned char * ) Buffer, palette,
							 IE_FONT_ALIGN_LEFT, finit, Cursor, pos );
		free( Buffer );
		video->SetClipRect( NULL );
		//streaming text
		if (linesize>50) {
			//the buffer is filled enough
			return;
		}

		AppendText("\n",-1);
		return;
	}
	// normal scrolling textarea
	int rc = 0;
	int sr = startrow;
	unsigned int i;
	int yl;
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
		yl = ftext->maxHeight * (lrows[i]-sr);
		clip.y+=yl;
		clip.h-=yl;
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
		ftext->Print( clip, ( unsigned char * ) lines[i], pal,
			IE_FONT_ALIGN_LEFT, true );
		yl = ftext->maxHeight * lrows[i];
		clip.y+=yl;
		clip.h-=yl;

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
void TextArea::SetText(const char* text)
{
	if (!text[0]) {
		Clear();
	}

	int newlen = ( int ) strlen( text );

	if (lines.size() == 0) {
		char* str = (char *) malloc( newlen + 1 );
		memcpy( str, text, newlen + 1 );
		lines.push_back( str );
		lrows.push_back( 0 );
	} else {
		lines[0] = (char *) realloc( lines[0], newlen + 1 );
		memcpy( lines[0], text, newlen + 1 );
	}
	CurPos = newlen;
	CurLine = lines.size()-1;
	UpdateControls();
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

static char *note_const = NULL;
static const char inserted_crap[]="[/color][color=ffffff]";
#define CRAPLENGTH sizeof(inserted_crap)-1

void TextArea::SetNoteString(const char *s)
{
	free(note_const);
	if (s) {
		note_const = (char *) malloc(strlen(s)+5);
		sprintf(note_const, "\r\n\r\n%s", s);
	}
}

/** Appends a String to the current Text */
int TextArea::AppendText(const char* text, int pos)
{
	int ret = 0;
	if (pos >= ( int ) lines.size()) {
		return -1;
	}
	int newlen = ( int ) strlen( text );

	if (pos == -1) {
		const char *note = NULL;
		if (note_const) {
			note = strstr(text,note_const);
		}
		char *str;
		if (NULL == note) {
			str = (char *) malloc( newlen +1 );
			memcpy(str, text, newlen+1);
		}
		else {
			unsigned int notepos = (unsigned int) (note - text);
			str = (char *) malloc( newlen + CRAPLENGTH+1 );
			memcpy(str,text,notepos);
			memcpy(str+notepos,inserted_crap,CRAPLENGTH);
			memcpy(str+notepos+CRAPLENGTH, text+notepos, newlen-notepos+1);
		}
		lines.push_back( str );
		lrows.push_back( 0 );
		ret =(int) (lines.size() - 1);
	} else {
		int mylen = ( int ) strlen( lines[pos] );

		lines[pos] = (char *) realloc( lines[pos], mylen + newlen + 1 );
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

	GameControl* gc = core->GetGameControl();
	if (gc && gc->GetDialogueFlags()&DF_IN_DIALOG) {
		// This hack is to refresh the mouse cursor so that reply below cursor gets
		// highlighted during a dialog
		// FIXME: we check DF_IN_DIALOG here to avoid recurssion in the MessageWindowLogger, but what happens when an error happens during dialog?
		// I'm not super sure about how to avoid that. for now the logger will not log anything in dialog mode.
		int x,y;
		core->GetVideoDriver()->GetMousePos(x,y);
		core->GetEventMgr()->MouseMove(x,y);
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
	if (Flags & IE_GUI_TEXTAREA_EDITABLE) {
		if (Key >= 0x20) {
			Owner->Invalidate();
			Changed = true;
			int len = GetRowLength(CurLine);
			//print("len: %d Before: %s", len, lines[CurLine]);
			lines[CurLine] = (char *) realloc( lines[CurLine], len + 2 );
			for (int i = len; i > CurPos; i--) {
				lines[CurLine][i] = lines[CurLine][i - 1];
			}
			lines[CurLine][CurPos] = Key;
			lines[CurLine][len + 1] = 0;
			CurPos++;
			//print("pos: %d After: %s", CurPos, lines[CurLine]);
			CalcRowCount();
			RunEventHandler( TextAreaOnChange );
		}
		return;
	}

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
			gc->dialoghandler->EndDialog();
			return;
		}
		gc->dialoghandler->DialogChoose( idx );
	}
}

/** Special Key Press */
void TextArea::OnSpecialKeyPress(unsigned char Key)
{
	int len;
	int i;

	if (!(Flags&IE_GUI_TEXTAREA_EDITABLE)) {
		return;
	}
	Owner->Invalidate();
	Changed = true;
	switch (Key) {
		case GEM_HOME:
			CurPos = 0;
			CurLine = 0;
			break;
		case GEM_UP:
			if (CurLine) {
				CurLine--;
			}
			break;
		case GEM_DOWN:
			if (CurLine<lines.size()) {
				CurLine++;
			}
			break;
		case GEM_END:
			CurLine=lines.size()-1;
			CurPos = GetRowLength((unsigned int) CurLine);
			break;
		case GEM_LEFT:
			if (CurPos > 0) {
				CurPos--;
			} else {
				if (CurLine) {
					CurLine--;
					CurPos = GetRowLength(CurLine);
				}
			}
			break;
		case GEM_RIGHT:
			len = GetRowLength(CurLine);
			if (CurPos < len) {
				CurPos++;
			} else {
				if(CurLine<lines.size()) {
					CurPos=0;
					CurLine++;
				}
			}
			break;
		case GEM_DELETE:
			len = GetRowLength(CurLine);
			//print("len: %d Before: %s", len, lines[CurLine]);
			if (CurPos>=len) {
				//TODO: merge next line
				break;
			}
			lines[CurLine] = (char *) realloc( lines[CurLine], len );
			for (i = CurPos; i < len; i++) {
				lines[CurLine][i] = lines[CurLine][i + 1];
			}
			//print("pos: %d After: %s", CurPos, lines[CurLine]);
			break;
		case GEM_BACKSP:
			len = GetRowLength(CurLine);
			if (CurPos != 0) {
				//print("len: %d Before: %s", len, lines[CurLine]);
				if (len<1) {
					break;
				}
				lines[CurLine] = (char *) realloc( lines[CurLine], len );
				for (i = CurPos; i < len; i++) {
					lines[CurLine][i - 1] = lines[CurLine][i];
				}
				lines[CurLine][len - 1] = 0;
				CurPos--;
				//print("pos: %d After: %s", CurPos, lines[CurLine]);
			} else {
				if (CurLine) {
					//TODO: merge lines
					int oldline = CurLine;
					CurLine--;
					int old = GetRowLength(CurLine);
					//print("len: %d Before: %s", old, lines[CurLine]);
					//print("len: %d Before: %s", len, lines[oldline]);
					lines[CurLine] = (char *) realloc (lines[CurLine], len+old);
					memcpy(lines[CurLine]+old, lines[oldline],len);
					free(lines[oldline]);
					lines[CurLine][old+len]=0;
					lines.erase(lines.begin()+oldline);
					lrows.erase(lrows.begin()+oldline);
					CurPos = old;
					//print("pos: %d len: %d After: %s", CurPos, GetRowLength(CurLine), lines[CurLine]);
				}
			}
			break;
		 case GEM_RETURN:
			//add an empty line after CurLine
			//print("pos: %d Before: %s", CurPos, lines[CurLine]);
			lrows.insert(lrows.begin()+CurLine, 0);
			len = GetRowLength(CurLine);
			//copy the text after the cursor into the new line
			char *str = (char *) malloc(len-CurPos+2);
			memcpy(str, lines[CurLine]+CurPos, len-CurPos+1);
			str[len-CurPos+1] = 0;
			lines.insert(lines.begin()+CurLine+1, str);
			//truncate the current line
			lines[CurLine] = (char *) realloc (lines[CurLine], CurPos+1);
			lines[CurLine][CurPos]=0;
			//move cursor to next line beginning
			CurLine++;
			CurPos=0;
			//print("len: %d After: %s", GetRowLength(CurLine-1), lines[CurLine-1]);
			//print("len: %d After: %s", GetRowLength(CurLine), lines[CurLine]);
			break;
	}
	CalcRowCount();
	RunEventHandler( TextAreaOnChange );
}

/** Returns Row count */
int TextArea::GetRowCount()
{
	return ( int ) lines.size();
}

int TextArea::GetRowLength(unsigned int row)
{
	if (lines.size()<=row) {
		return 0;
	}
	//this is just roughly the line size, escape sequences need to be removed
	return strlen( lines[row] );
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

int TextArea::GetRowHeight()
{
	return ftext->maxHeight;
}

/** Will scroll y pixels. sender is the control requesting the scroll (ie the scrollbar) */
void TextArea::ScrollToY(unsigned long y, Control* sender)
{
	if (sb && sender != sb) {
		// we must "scale" the pixels
		((ScrollBar*)sb)->SetPosForY(y * (((ScrollBar*)sb)->GetStep() / (double)ftext->maxHeight));
		// sb->SetPosForY will recall this method so we dont need to do more... yet.
	}else if(sb){
		// our scrollbar has set position for us
		TextYPos = y;
	}else{
		// no scrollbar. need to call SetRow myself.
		// SetRow will set TextYPos.
		SetRow( y / ftext->maxHeight );
	}
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row < rows) {
		startrow = row;
		TextYPos = row * ftext->maxHeight;
	}
	Changed = true;
}

void TextArea::CalcRowCount()
{
	int tr;
	int w = Width;

	if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
		const char *portrait = NULL;
		Actor *actor = NULL;
		GameControl *gc = core->GetGameControl();
		if (gc) {
			Scriptable *target = gc->dialoghandler->GetTarget();
			if (target && target->Type == ST_ACTOR) {
				actor = (Actor *)target;
			}
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
//			rows++;
			tr = 0;
			int len = ( int ) strlen( lines[i] );
			char* tmp = (char *) malloc( len + 1 );
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
//					if (p != len)
//						rows++;
					tr++;
				}
			}
			lrows[i] = tr;
			rows += tr;
			free( tmp );
		}
	}

	if (lines.size())
	{
		if (CurLine>=lines.size()) {
			CurLine=lines.size()-1;
		}
		w = strlen(lines[CurLine]);
		if (CurPos>w) {
			CurPos = w;
		}
	} else {
		CurLine=0;
		CurPos=0;
	}

	if (!sb) {
		return;
	}
	ScrollBar* bar = ( ScrollBar* ) sb;
	tr = rows - Height/ftext->maxHeight + 1;
	if (tr<0) {
		tr = 0;
	}
	bar->SetMax( (ieWord) tr );
}

/** Mousewheel scroll */
/** This method is key to touchscreen scrolling */
void TextArea::OnMouseWheelScroll(short /*x*/, short y)
{
	if (!(IE_GUI_TEXTAREA_SMOOTHSCROLL & Flags)){
		unsigned long fauxY = TextYPos;
		if ((long)fauxY + y <= 0) fauxY = 0;
		else fauxY += y;
		ScrollToY(fauxY, this);
	}
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
			//print("CtrlId = 0x%08lx, seltext = %d, rows = %d, row = %d, r = %d", ControlID, i, rows, row, r);
			return;
		}
	}
	if (seltext != -1) {
		core->RedrawAll();
	}
	seltext = -1;
	//print("CtrlId = 0x%08lx, seltext = %d, rows %d, row %d, r = %d", ControlID, seltext, rows, row, r);
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short x, unsigned short y, unsigned short /*Button*/,
	unsigned short /*Mod*/)
{
	if (( x <= Width ) && ( y <= ( Height - 5 ) ) && ( seltext != -1 )) {
		Value = (unsigned int) seltext;
		Changed = true;
		if (strnicmp( lines[seltext], "[s=", 3 ) == 0) {
			if (minrow > seltext)
				return;
			int idx;
			sscanf( lines[seltext], "[s=%d,", &idx );
			GameControl* gc = core->GetGameControl();
			if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
				if (idx==-1) {
					//this kills this object, don't use any more data!
					gc->dialoghandler->EndDialog();
					return;
				}
				gc->dialoghandler->DialogChoose( idx );
				return;
			}
		}
	}

	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}
	RunEventHandler( TextAreaOnChange );
}

void TextArea::SetText(const std::vector<char*>& text)
{
	Clear();
	for (size_t i = 0; i < text.size(); i++) {
		int newlen = strlen(text[i]);
		char* str = (char *) malloc(newlen + 1);
		memcpy(str, text[i], newlen + 1);
		lines.push_back(str);
		lrows.push_back(0);
		CurPos = newlen;
	}
	CurLine = lines.size() - 1;
	UpdateControls();
}

/** Copies the current TextArea content to another TextArea control */
void TextArea::CopyTo(TextArea *ta)
{
	ta->SetText(lines);
}

void TextArea::RedrawTextArea(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	Value = Sum;
	Changed = true;
}

void TextArea::SelectText(const char *select)
{
	int i = lines.size();
	while(i--) {
		if (!stricmp(lines[i], select) ) {
			CurLine = i;
			if (sb) {
				ScrollBar* bar = ( ScrollBar* ) sb;
				bar->SetPos( i );
			} else {
				SetRow( i );
			}
			RedrawTextArea( VarName, i);
			CalcRowCount();
			Owner->Invalidate();
			core->RedrawAll();
			break;
		}
	}
}

const char* TextArea::QueryText()
{
	if ( Value<lines.size() ) {
		return ( const char * ) lines[Value];
	}
	return ( const char *) "";
}

bool TextArea::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		TextAreaOnChange = handler;
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
void TextArea::SetupScroll()
{
	SetPreservedRow(0);
	startrow = 0;
	// ticks is the number of ticks it takes to scroll this font 1 px
	ticks = 2400 / ftext->maxHeight;
	//clearing the textarea
	Clear();
	unsigned int i = (unsigned int) (1 + ((Height - 1) / ftext->maxHeight)); // ceiling
	while (i--) { //push empty lines so that the text starts out of view.
		char *str = (char *) malloc(1);
		str[0]=0;
		lines.push_back(str);
		lrows.push_back(0);
	}
	i = (unsigned int) lines.size();
	Flags |= IE_GUI_TEXTAREA_SMOOTHSCROLL;
	starttime = GetTickCount();
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
			core->RedrawAll();
			break;
		case GEM_MB_SCRLDOWN:
			scrlbr->ScrollDown();
			core->RedrawAll();
			break;
		}
	}
}

void TextArea::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus && Flags & IE_GUI_TEXTAREA_EDITABLE) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

}
