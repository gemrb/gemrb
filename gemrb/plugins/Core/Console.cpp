#include "../../includes/win32def.h"
#include "Interface.h"
#include "Console.h"

extern Interface * core;

Console::Console(void)
{
	ta = NULL;
	Buffer = (unsigned char*)malloc(1024);
	Buffer[0] = 0;
	CurPos = 0;
	Changed = true;
}

Console::~Console(void)
{
	free(Buffer);
}

/** Draws the Console on the Output Display */
void Console::Draw(unsigned short x, unsigned short y)
{
	if(ta)
		ta->Draw(x,y-ta->Height);
	if(!Changed)
		return;
	Color black = {0x00, 0x00, 0x00, 0x00};
	core->GetVideoDriver()->DrawRect(Region(x+XPos, y+YPos, Width, Height), black);
	font->Print(Region(x+XPos, y+YPos, Width, Height), Buffer, NULL, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true, NULL, NULL, Cursor, CurPos);
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
int Console::SetText(const char * string)
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
			Buffer[CurPos] = 0;
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
}
/** Special Key Press */
void Console::OnSpecialKeyPress(unsigned char Key)
{
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
	else if(Key == GEM_RETURN) {
		char * msg = core->GetGUIScriptEngine()->ExecString((char*)Buffer);
		if(ta)
			ta->SetText(msg);
		free(msg);
		Buffer[0] = 0;
		CurPos = 0;
		Changed = true;
	}
}