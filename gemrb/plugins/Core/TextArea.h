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
	void SetScrollBar(Control * ptr);
	/** Sets the Actual Text */
	int SetText(const char * text, int pos = 0);
	/** Appends a String to the current Text */
	int AppendText(const char * text, int pos = 0);
	/** Sets the Fonts */
	void SetFonts(Font * init, Font * text);
	/** Returns Number of Rows */
	int GetRowCount();
	/** Returns Starting Row */
	int GetTopIndex();
	/** Set Starting Row */
	void SetRow(int row);
	/** Set Selectable */
	void SetSelectable(bool val);
	bool Selectable;
private: // Private attributes
	std::vector<char *> lines;
	std::vector<int> lrows;
	int seltext;
	///** Text Buffer */
	//unsigned char * Buffer;
	/** Number of Text Rows */
	int rows;
	/** Starting Row */
	int startrow;
	/** Attached Scroll Bar Pointer*/
	Control * sb;
	/** Text Colors */
	Color * palette;
	Color * initpalette;
	Color * selected;
	/** Fonts */
	Font *finit, *ftext;
	void CalcRowCount();
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
};

#endif
