#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "Control.h"
#include "Sprite2D.h"

#define IE_GUI_SCROLLBAR_UP_UNPRESSED   0
#define IE_GUI_SCROLLBAR_UP_PRESSED     1
#define IE_GUI_SCROLLBAR_DOWN_UNPRESSED 2
#define IE_GUI_SCROLLBAR_DOWN_PRESSED   3
#define IE_GUI_SCROLLBAR_TROUGH         4
#define IE_GUI_SCROLLBAR_SLIDER         5

#define UP_PRESS     0x0001
#define DOWN_PRESS   0x0010
#define SLIDER_GRAB  0x0100

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ScrollBar : public Control
{
public:
	ScrollBar(void);
	~ScrollBar(void);
	void Draw(unsigned short x, unsigned short y);
private: //Private attributes
	/** Images for drawing the Scroll Bar */
	Sprite2D * frames[6];
	/** Cursor Position */
	unsigned short Pos;
	/** Cursor Max Position */
	unsigned short Max;
	/** Scroll Bar Status */
	unsigned short State;
	/** Sets the Text of the current control */
	int SetText(const char * string);
public:
	void SetImage(unsigned char type, Sprite2D * img);
public: // Public Events
  /** Mouse Button Down */
  void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Button Up */
  void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Over Event */
  void OnMouseOver(unsigned short x, unsigned short y);
};

#endif
