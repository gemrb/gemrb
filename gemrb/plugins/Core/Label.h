/***************************************************************************
                          Label.h  -  description
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

#ifndef LABEL_H
#define LABEL_H

#include "Control.h"
#include "Font.h"
#include "../../includes/RGBAColor.h"

/**Static Text GUI Control
  *@author GemRB Developement Team
  */

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Label : public Control  {
public: 
	Label(unsigned short bLength, Font * font);
	~Label();
  /** Draws the Control on the Output Display */
  void Draw(unsigned short x, unsigned short y);
  /** This function sets the actual Label Text */
  void SetText(char * string);
  /** Sets the Foreground Font Color */
  void SetColor(Color col);
private: // Private attributes
  /** Text String Buffer */
  char * Buffer;
  /** Font for Text Writing */
  Font * font;
  /** Foregroun Color */
  Color fore;
  /** Use the RGB Color for the Font */
  bool useRGB;
};

#endif
