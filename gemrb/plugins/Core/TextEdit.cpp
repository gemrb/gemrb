#include "../../includes/win32def.h"
#include "TextEdit.h"
#include "Interface.h"

extern Interface * core;

TextEdit::TextEdit(unsigned short maxLength)
{
	max = maxLength;
	Buffer = (unsigned char*)malloc(max+1);
	font = NULL;
	Cursor = NULL;
	Back = NULL;
	CurPos = 0;
	strncpy((char*)Buffer, "Text Edit", max);
	Color white = {0xff, 0xff, 0xff, 0x00}, black = {0x00, 0x00, 0x00, 0x00};
	palette = core->GetVideoDriver()->CreatePalette(white, black);
}

TextEdit::~TextEdit(void)
{
	free(palette);
	free(Buffer);
}

/** Draws the Control on the Output Display */
void TextEdit::Draw(unsigned short x, unsigned short y)
{
	if(!Changed)
		return;
	if(hasFocus)
		font->Print(Region(x+XPos, y+YPos, Width, Height), Buffer, palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true, NULL, NULL, Cursor, CurPos);
	else
		font->Print(Region(x+XPos, y+YPos, Width, Height), Buffer, palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true);
	Changed = false;
}

/** Set Font */
void TextEdit::SetFont(Font * f)
{
	if(f != NULL)
		font = f;
	Changed = true;
}

/** Set Cursor */
void TextEdit::SetCursor(Sprite2D * cur)
{
	if(cur != NULL)
		Cursor = cur;
	Changed = true;
}

/** Set BackGround */
void TextEdit::SetBackGround(Sprite2D * back)
{
	//if 'back' is NULL then no BackGround will be drawn
	Back = back;
	Changed = true;
}

/** Key Press Event */
void TextEdit::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	((Window*)Owner)->Invalidate();
	Changed = true;
	if(Key >= 0x20) {
		int len = strlen((char*)Buffer);
		if(len+1 < max) {
			for(int i = len; i > CurPos; i--) {
				Buffer[i] = Buffer[i-1];
			}
			Buffer[CurPos] = Key;
			CurPos++;
		}
	}
	else if(Key == '\b') {
		if(CurPos != 0) {
			int len = strlen((char*)Buffer);
			for(int i = CurPos; i < len; i++) {
				Buffer[i-1] = Buffer[i];
			}
			Buffer[len-1] = 0;
			CurPos--;
		}
	}
	//else if(key == '     
}
/** Special Key Press */
void TextEdit::OnSpecialKeyPress(unsigned char Key)
{
	((Window*)Owner)->Invalidate();
	Changed = true;
	if(Key == GEM_LEFT) {
		if(CurPos > 0)
			CurPos--;
	}
	else if(Key == GEM_RIGHT) {
		int len = strlen((char*)Buffer);
		if(CurPos < len) {
			CurPos++;
		}
	}
	else if(Key == GEM_DELETE) {
		int len = strlen((char*)Buffer);
		if(CurPos < len) {
			for(int i = CurPos; i < len; i++) {
				Buffer[i] = Buffer[i+1];
			}
		}			
	}
}

/** Sets the Text of the current control */
int TextEdit::SetText(const char * string)
{
	strncpy((char*)Buffer, string, max);
	((Window*)Owner)->Invalidate();
	return 0;
}
