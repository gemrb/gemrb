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
public:
	void SetImage(unsigned char type, Sprite2D * img);
};

#endif
