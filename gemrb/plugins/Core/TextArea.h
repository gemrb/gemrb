/***************************************************************************
                          TextArea.h  -  description
                             -------------------
    begin                : dom ott 12 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "Control.h"
#include "../../includes/RGBAColor.h"
#include "Font.h"
#include "ScrollBar.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TextArea : public Control
{
public:
	TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Scroll Bar Pointer */
	void SetScrollBar(ScrollBar * ptr);
	/** Sets the Actual Text */
	void SetText(unsigned char * text);
	/** Sets the Fonts */
	void SetFonts(Font * init, Font * text);
private: // Private attributes
	/** Text Buffer */
	unsigned char * Buffer;
	/** Attached Scroll Bar Pointer*/
	ScrollBar * sb;
	/** Text Colors */
	Color hi, init, low;
	/** Fonts */
	Font *finit, *ftext;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
};

#endif
