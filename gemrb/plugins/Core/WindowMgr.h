/***************************************************************************
                          WindowMgr.h  -  description
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

#ifndef WINDOWMGR_H
#define WINDOWMGR_H

#include "Plugin.h"
#include "Window.h"
#include "DataStream.h"

/**This Class Defines basic Methods for the implementation of a GUI Window Manager.
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

class GEM_EXPORT WindowMgr : public Plugin  {
public: 
	WindowMgr();
	virtual ~WindowMgr();
  /** This function loads all available windows from the 'stream' parameter. */
  virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
  /** Returns the i-th window in the Previously Loaded Stream */
  virtual Window * GetWindow(unsigned long i) = 0;
  /** Returns the number of available windows */
  virtual unsigned long GetWindowsCount() = 0;
};

#endif
