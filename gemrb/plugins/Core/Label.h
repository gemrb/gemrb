/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Label.h,v 1.8 2003/11/25 13:48:02 balrog994 Exp $
 *
 */

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
  int SetText(const char * string, int pos = 0);
  /** Sets the Foreground Font Color */
  void SetColor(Color col, Color bac);
  /** Sets the Alignment of Text */
  void SetAlignment(unsigned char Alignment);
  /** Use the RGB Color for the Font */
  bool useRGB;
private: // Private attributes
  /** Text String Buffer */
  char * Buffer;
  /** Font for Text Writing */
  Font * font;
  /** Foreground & Background Colors */
  Color *palette;
  
  /** Alignment Variable */
  unsigned char Alignment;
};

#endif
