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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EventMgr.cpp,v 1.21 2003/12/18 15:05:21 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "EventMgr.h"

EventMgr::EventMgr(void)
{
	lastW = NULL;
	lastF = NULL;
}

EventMgr::~EventMgr(void)
{
}

/** Adds a Window to the Event Manager */
void EventMgr::AddWindow(Window * win)
{
	if(win == NULL)
		return;
	for(unsigned int i = 0; i < windows.size(); i++) {
		if(windows[i] == win) {
			SetOnTop(i);
			return;
		}
	}
	bool found = false;
	for(unsigned int i = 0; i < windows.size(); i++) {
		if(windows[i] == NULL) {
			windows[i] = win;
			SetOnTop(i);
			found = true;
			break;
		}
	}
	if(!found) {
		windows.push_back(win);
		if(windows.size() == 1)
			topwin.push_back(0);
		else
			SetOnTop((int)windows.size()-1);
	}
	lastW = win;
	lastF = NULL;
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	topwin.clear();
	std::vector<Window*>::iterator m;
	for(m = windows.begin(); m != windows.end(); ++m) {
		(*m) = NULL;
	}
	lastW = NULL;
	lastF = NULL;
}

/** Remove a Window fro the array */
void EventMgr::DelWindow(unsigned short WindowID)
{
	if(windows.size() == 0)
		return;
	int pos = -1;
	std::vector<Window*>::iterator m;
	for(m = windows.begin(); m != windows.end(); ++m) {
		pos++;
		if((*m) == NULL)
			continue;
		if((*m)->WindowID == WindowID) {
			if(lastW == (*m))
				lastW = NULL;
			//windows.erase(m);
			(*m) = NULL;
			lastF = NULL;
			break;
		}
	}
	if(pos != -1) {
		std::vector<int>::iterator t;
		for(t = topwin.begin(); t != topwin.end(); ++t) {
			if((*t) == pos) {
				topwin.erase(t);
				break;
			}
		}
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	if(windows.size() == 0)
		return;
	std::vector<int>::iterator t;
	std::vector<Window*>::iterator m;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m+=(*t);
		if((*m) == NULL)
			continue;
		if(!(*m)->Visible)
			continue;
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(ctrl != NULL) {
					ctrl->OnMouseOver(x-lastW->XPos-ctrl->XPos,y-lastW->YPos-ctrl->YPos);
				}
				lastW = *m;
				return;
			}	
		}
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	std::vector<int>::iterator t;
	std::vector<Window*>::iterator m;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m+=(*t);
		if((*m) == NULL)
			continue;
		if(!(*m)->Visible)
			continue;
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(lastW == NULL)
					lastW = (*m);
				if(ctrl != NULL) {
					(*m)->SetFocused(ctrl);
					ctrl->OnMouseDown(x-lastW->XPos-ctrl->XPos,y-lastW->YPos-ctrl->YPos, Button, Mod);
					lastF = ctrl;
				}
				lastW = *m;
				return;
			}	
		}
	}
	if(lastF)
		lastF->hasFocus = false;
	lastF = NULL;
}
/** BroadCast Mouse Move Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	if(lastF != NULL) {
		lastF->OnMouseUp(x-lastW->XPos-lastF->XPos, y-lastW->YPos-lastF->YPos, Button, Mod);
	}
}

/** BroadCast Key Press Event */
void EventMgr::KeyPress(unsigned char Key, unsigned short Mod)
{
	if(lastF)
		lastF->OnKeyPress(Key, Mod);
}
/** BroadCast Key Release Event */
void EventMgr::KeyRelease(unsigned char Key, unsigned short Mod)
{
	if(lastF)
		lastF->OnKeyRelease(Key, Mod);
}

/** Special Ket Press Event */
void EventMgr::OnSpecialKeyPress(unsigned char Key)
{
	if(lastW) {
		Control* ctrl = lastW->GetControl(0);
		if(!ctrl) {
			if(lastF)
				lastF->OnSpecialKeyPress(Key);
		}
		if(ctrl->ControlType != IE_GUI_GAMECONTROL) {
			if(lastF)
				lastF->OnSpecialKeyPress(Key);
		}
		ctrl->OnSpecialKeyPress(Key);
		return;
	}
	if(lastF)
		lastF->OnSpecialKeyPress(Key);
}
