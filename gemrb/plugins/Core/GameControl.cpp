#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"

extern Interface * core;

GameControl::GameControl(void)
{
	area = NULL;
	Changed = true;
}

GameControl::~GameControl(void)
{
}

/** Draws the Control on the Output Display */
void GameControl::Draw(unsigned short x, unsigned short y)
{
	Region vp(x+XPos, y+YPos, Width, Height);
	if(area)
		area->DrawMap(vp);
	else {
		Color Blue;
		Blue.b = 0xff;
		Blue.r = 0;
		Blue.g = 0;
		Blue.a = 128;
		core->GetVideoDriver()->DrawRect(vp, Blue);
	}
}
/** Sets the Text of the current control */
int GameControl::SetText(const char * string, int pos)
{
	return 0;
}
/** Key Press Event */
void GameControl::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	printf("KeyPress\n");
}
/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	printf("KeyRelease\n");
}
/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	printf("MouseMove\n");
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	printf("MouseDown\n");
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	printf("MouseUp\n");
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{	
	printf("SpecialKeyPress\n");
}
