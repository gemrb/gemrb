#ifndef CONSOLE_H
#define CONSOLE_H

#include "Control.h"
#include "TextArea.h"

class Console :
	public Control
{
public:
	Console(void);
	~Console(void);
	/** Draws the Console on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Set Font */
	void SetFont(Font * f);
	/** Set Cursor */
	void SetCursor(Sprite2D * cur);
	/** Set BackGround */
	void SetBackGround(Sprite2D * back);
	/** Sets the Text of the current control */
	int SetText(const char * string);
	/** Output for Script Messages */
	TextArea * ta;
private:
	/** Text Editing Cursor Sprite */
	Sprite2D * Cursor;
	/** Text Font */
	Font * font;
	/** Background */
	Sprite2D * Back;
	/** Max Edit Text Length */
	unsigned short max;
	/** Text Buffer */
	unsigned char * Buffer;
	/** Cursor Position */
	unsigned short CurPos;
	/** Color Palette */
	Color * palette;
	
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
};

#endif
