#include "../../includes/win32def.h"
#include "Interface.h"
#include "Console.h"

extern Interface * core;

Console::Console(void)
{
	ta = NULL;
	Cursor = NULL;
	max = 128;
	Buffer = (unsigned char*)malloc(max);
	Buffer[0] = 0;
	CurPos = 0;
	Changed = true;
	if(stricmp(core->GameType, "iwd2") == 0) {
		Color fore = {0xff, 0xff, 0xff, 0x00}, back = {0x00, 0x00, 0x00, 0x00};
		palette = core->GetVideoDriver()->CreatePalette(fore, back);
	}
	else
		palette = NULL;
}

Console::~Console(void)
{
	free(Buffer);
	if(palette)
		free(palette);
}

/** Draws the Console on the Output Display */
void Console::Draw(unsigned short x, unsigned short y)
{
	if(ta)
		ta->Draw(x,y-ta->Height);
	if(!Changed)
		return;
	Color black = {0x00, 0x00, 0x00, 0x00};
	Region r(x+XPos, y+YPos, Width, Height);
	core->GetVideoDriver()->DrawRect(r, black);
	font->Print(r, Buffer, palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true, NULL, NULL, Cursor, CurPos);
	Changed = false;
}
/** Set Font */
void Console::SetFont(Font * f)
{	
	if(f != NULL)
		font = f;
	Changed = true;
}
/** Set Cursor */
void Console::SetCursor(Sprite2D * cur)
{
	if(cur != NULL)
		Cursor = cur;
	Changed = true;
}
/** Set BackGround */
void Console::SetBackGround(Sprite2D * back)
{
	//if 'back' is NULL then no BackGround will be drawn
	Back = back;
	Changed = true;
}
/** Sets the Text of the current control */
int Console::SetText(const char * string, int pos)
{	
	strncpy((char*)Buffer, string, max);
	Changed = true;
	return 0;
}
/** Key Press Event */
void Console::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	Changed = true;
	if(Key >= 0x20) {
		int len = strlen((char*)Buffer);
		if(len+1 < max) {
			for(int i = len; i > CurPos; i--) {
				Buffer[i] = Buffer[i-1];
			}
			Buffer[CurPos++] = Key;
			Buffer[len+1] = 0;
		}
	}
}
/** Special Key Press */
void Console::OnSpecialKeyPress(unsigned char Key)
{
	int len;

	Changed = true;
	switch(Key)
	{
	case GEM_BACKSP:
		if(CurPos != 0) {
			int len = strlen((char*)Buffer);
			for(int i = CurPos; i < len; i++) {
				Buffer[i-1] = Buffer[i];
			}
			Buffer[len-1] = 0;
			CurPos--;
		}
		break;
	case GEM_LEFT:
		if(CurPos > 0)
			CurPos--;
		break;
	case GEM_RIGHT:
		len = strlen((char*)Buffer);
		if(CurPos < len) {
			CurPos++;
		}
		break;
	case GEM_DELETE:
		len = strlen((char*)Buffer);
		if(CurPos < len) {
			for(int i = CurPos; i < len; i++) {
				Buffer[i] = Buffer[i+1];
			}
		}
		break;			
	case GEM_RETURN:
		char * msg = core->GetGUIScriptEngine()->ExecString((char*)Buffer);
		if(ta)
			ta->SetText(msg);
		free(msg);
		Buffer[0] = 0;
		CurPos = 0;
		Changed = true;
		break;
	}
}
