/***************************************************************************
                          Window.h  -  description
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

#ifndef WINDOW_H
#define WINDOW_H

#include "Sprite2D.h"
#include "Control.h"
#include <vector>

/**This class defines a Window as an Items Container.
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

class GEM_EXPORT Window {
public: 
	Window(unsigned short WindowID, unsigned short XPos, unsigned short YPos, unsigned short Width, unsigned short Height);
	~Window();
  /** Set the Window's BackGround Image. If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
  void SetBackGround(Sprite2D * img, bool clean = false);
  /** Add a Control in the Window */
  void AddControl(Control * ctrl);
  /** This function Draws the Window on the Output Screen */
  void DrawWindow();
  /** Returns the Control at X,Y Coordinates */
  Control * GetControl(unsigned short x, unsigned short y);
  /** Returns the Control by Index */
  Control * GetControl(unsigned short i);
  /** Sets 'ctrl' as Focused */
  void SetFocused(Control * ctrl);
public: //Public attributes
  /** Window ID */
  unsigned short WindowID;
  /** X Position */
  unsigned short XPos;
  /** Y Position */
  unsigned short YPos;
  /** Width */
  unsigned short Width;
  /** Height */
  unsigned short Height;
private: // Private attributes
  /** BackGround Image. No BackGround if this variable is NULL. */
  Sprite2D * BackGround;
  /** Controls Array */
  std::vector<Control*> Controls;
  /** Last Control returned by GetControl */
  Control * lastC;
  /** Last Focused Control */
  Control * lastFocus;
public:
	void release(void);
};

#endif
