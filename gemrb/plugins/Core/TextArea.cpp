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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TextArea.cpp,v 1.63 2004/11/18 23:32:41 edheldil Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TextArea.h"
#include "Interface.h"
#include <stdio.h>
#include <stdlib.h>

TextArea::TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor)
{
	rows = 0;
	startrow = 0;
	minrow = 0;
	seltext = -1;
	Value = 0xffffffff;
	sb = NULL;
	Selectable = false;
	AutoScroll = false;
	ResetEventHandler( TextAreaOnChange );

	palette = core->GetVideoDriver()->CreatePalette( hitextcolor,
										lowtextcolor );
	initpalette = core->GetVideoDriver()->CreatePalette( initcolor,
											lowtextcolor );
	Color tmp = {
		hitextcolor.b, hitextcolor.g, hitextcolor.r, 0
	};
	selected = core->GetVideoDriver()->CreatePalette( tmp, lowtextcolor );
	tmp.r = 255;
	tmp.g = 152;
	tmp.b = 102;
	lineselpal = core->GetVideoDriver()->CreatePalette( tmp, lowtextcolor );
}

TextArea::~TextArea(void)
{
	Video *video = core->GetVideoDriver();
	video->FreePalette( palette );
	video->FreePalette( initpalette );
	video->FreePalette( selected );
	video->FreePalette( lineselpal );
	for (size_t i = 0; i < lines.size(); i++) {
		free( lines[i] );
	}
}

void TextArea::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !((Window*)Owner)->Floating) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	if (lines.size() == 0) {
		return;
	}
	if (!Selectable) {
		char* Buffer = ( char* ) malloc( 1 );
		Buffer[0] = 0;
		int len = 0;
		int lastlen = 0;
		for (size_t i = 0; i < lines.size(); i++) {
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
			if (i != lines.size() - 1) {
				Buffer[lastlen - 1] = '\n';
				Buffer[lastlen] = 0;
			}
		}
		ftext->PrintFromLine( startrow,
				Region( x + XPos, y + YPos, Width, Height - 5 ),
				( unsigned char * ) Buffer, palette, IE_FONT_ALIGN_LEFT, true,
				finit, initpalette );
		free( Buffer );
	} else {
		int rc = 0;
		int sr = startrow;
		unsigned int i;
		int yl = 0;
		for (i = 0; i < lines.size(); i++) {
			if (rc + lrows[i] <= sr) {
				rc += lrows[i];
				continue;
			}
			sr -= rc;
			Color* pal = NULL;
			if (seltext == (int) i)
				pal = selected;
			else if (Value == i)
				pal = lineselpal;
			else
				pal = palette;
			ftext->PrintFromLine( sr,
					Region( x + XPos, y + YPos, Width, Height - 5 ),
					( unsigned char * ) lines[i], pal, IE_FONT_ALIGN_LEFT,
					true, finit, initpalette );
			yl = lrows[i] - sr;
			break;
		}
		for (i++; i < lines.size(); i++) {
			Color* pal = NULL;
			if (seltext == (int) i)
				pal = selected;
			else if (Value == i)
				pal = lineselpal;
			else
				pal = palette;
			ftext->Print( Region( x + XPos, y + YPos +
					( yl * ftext->size[1].h/*chars[1]->Height*/ ), Width,
					Height - 5 - ( yl * ftext->size[1].h/*chars[1]->Height*/ ) ),
					( unsigned char * ) lines[i], pal, IE_FONT_ALIGN_LEFT, true );
			yl += lrows[i];
		}
	}
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Text Area Control. */
void TextArea::SetScrollBar(Control* ptr)
{
	sb = ptr;
	CalcRowCount();
	Changed = true;
}
/** Sets the Actual Text */
int TextArea::SetText(const char* text, int pos)
{
	if (( pos == 0 ) && ( lines.size() == 0 )) {
		pos = -1;
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
	CalcRowCount();
	Changed = true;
	if (sb) {
		ScrollBar* bar = ( ScrollBar* ) sb;
		if (AutoScroll)
			pos = rows - ( ( Height - 5 ) / ftext->maxHeight );
		else
			pos = 0;
		//pos=lines.size()-((Height-5)/ftext->maxHeight);
		if (pos < 0)
			pos = 0;
		bar->SetPos( pos );
	}
	core->RedrawAll();
	return 0;
}

void TextArea::SetMinRow(bool enable)
{
	if (enable) {
		minrow = lines.size();
	} else {
		minrow = 0;
	}
	Changed = true;
}

static char inserted_crap[]="[/color][color=ffffff]";
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
		char *note = strstr(text,"\r\n\r\nNOTE:");
		char *str;
		if(NULL == note) {
			str = ( char* ) malloc( newlen +1 );
			memcpy(str,text, newlen+1);
		}
		else {
			unsigned int notepos = note - text;
			str = ( char* ) malloc( newlen + CRAPLENGTH+1 );
			memcpy(str,text,notepos);
			memcpy(str+notepos,inserted_crap,CRAPLENGTH);
			memcpy(str+notepos+CRAPLENGTH, text+notepos, newlen-notepos+1);
		}
		lines.push_back( str );
		lrows.push_back( 0 );
		ret = lines.size() - 1;
	} else {
		int mylen = ( int ) strlen( lines[pos] );

		lines[pos] = ( char * ) realloc( lines[pos], mylen + newlen + 1 );
		memcpy( lines[pos]+mylen, text, newlen + 1 );
		ret = pos;
	}
	CalcRowCount();
	Changed = true;
	if (sb) {
		ScrollBar* bar = ( ScrollBar* ) sb;
		if (AutoScroll)
			pos = rows - ( ( Height - 5 ) / ftext->maxHeight );
		else
			pos = 0;
		//pos=lines.size()-((Height-5)/ftext->maxHeight);
		if (pos < 0)
			pos = 0;
		bar->SetPos( pos );
	}
	core->RedrawAll();
	return ret;
}

/** Deletes last `count' lines */ 
void TextArea::PopLines(unsigned int count)
{
	if (count > lines.size()) {
		count = lines.size();
	}

	while (count > 0 ) {
		free(lines.back() );
		lines.pop_back();
		lrows.pop_back();
		count--;
	}

	int pos;
	CalcRowCount();
	Changed = true;
	if (sb) {
		ScrollBar* bar = ( ScrollBar* ) sb;
		if (AutoScroll)
			pos = rows - ( ( Height - 5 ) / ftext->maxHeight );
		else
			pos = 0;
		if (pos < 0) {
			pos = 0;
		}
		bar->SetPos( pos );
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
	if (( Key >= '1' ) && ( Key <= '9' )) {
		//Actually selectable=false for dialogs
		if (!Selectable) {
			Window* win = core->GetWindow( 0 );
			if (win) {
				GameControl* gc = ( GameControl* ) win->GetControl( 0 );
				if (gc->ControlType == IE_GUI_GAMECONTROL) {
					if (gc->DialogueFlags&DF_IN_DIALOG) {
						Changed = true;
//FIXME: this should choose only valid options
						seltext=minrow-1;
						for(int i=0;i<Key-'0';i++) {
							do {
								seltext++;
							}
							while(((unsigned int) seltext<lines.size()) && (strnicmp( lines[seltext], "[s=", 3 ) != 0) );
							if((unsigned int) seltext>=lines.size()) {
								return;
							}
						}
						unsigned long idx=0;
						sscanf( lines[seltext], "[s=%lu,", &idx);

						gc->DialogChoose( idx );
					}
				}
			}
		}
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

/** Set Selectable */
void TextArea::SetSelectable(bool val)
{
	Selectable = val;
	if (Selectable) {
		minrow = 0;
	}
}

void TextArea::CalcRowCount()
{
	if (lines.size() != 0) {
		rows = 0;
		for (size_t i = 0; i < lines.size(); i++) {
			rows++;
			int tr = 0;
			int len = ( int ) strlen( lines[i] );
			char* tmp = ( char* ) malloc( len + 1 );
			memcpy( tmp, lines[i], len + 1 );
			ftext->SetupString( tmp, Width );
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
	bar->SetMax( rows );
}
/** Mouse Over Event */
void TextArea::OnMouseOver(unsigned short /*x*/, unsigned short y)
{
	int height = ftext->size[1].h;//ftext->chars[1]->Height;
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
			unsigned long idx;
			sscanf( lines[seltext], "[s=%lu,", &idx );
			Window* win = core->GetWindow( 0 );
			if (win) {
				GameControl* gc = ( GameControl* ) win->GetControl( 0 );
				if (gc->ControlType == IE_GUI_GAMECONTROL) {
					if (gc->DialogueFlags&DF_IN_DIALOG) {
						gc->DialogChoose( idx );
					}

				}
			}
		}
		core->RedrawAll();
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}
	RunEventHandler( TextAreaOnChange );
}

/** Copies the current TextArea content to another TextArea control */
void TextArea::CopyTo(TextArea* ta)
{
	for (size_t i = 0; i < lines.size(); i++) {
		ta->SetText( lines[i], -1 );
	}
}

void TextArea::RedrawTextArea(char* VariableName, unsigned int Sum)
{
        if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
                return;
        }
	Value = Sum;
	Changed = true;
}

const char* TextArea::QueryText()
{
	if( Value<lines.size() ) {
	        return ( const char * ) lines[Value];
	}
	return ( const char *) "";
}

bool TextArea::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		SetEventHandler( TextAreaOnChange, handler );
		break;
	default:
		return Control::SetEvent( eventType, handler );
	}

	return true;
}
