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
	windows.push_back(win);
	if(windows.size() == 1)
		topwin.push_back(0);
	else
		SetOnTop(windows.size()-1);
	lastW = win;
	lastF = NULL;
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	topwin.clear();
	std::vector<Window*>::iterator m;
	while(windows.size() != 0) {
		m = windows.begin();
		windows.erase(m);
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
		if((*m)->WindowID == WindowID) {
			if(lastW == (*m))
				lastW = NULL;
			windows.erase(m);
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
	/*if((lastW != NULL) && (lastW->Visible)) {
		if((lastW->XPos <= x) && (lastW->YPos <= y)) { //Maybe we are on the window, let's check
			if((lastW->XPos+lastW->Width >= x) && (lastW->YPos+lastW->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = lastW->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
						ctrl->OnMouseOver(x-lastW->XPos-ctrl->XPos,y-lastW->YPos-ctrl->YPos);
				}
				return;
			}	
		}
	}*/
	std::vector<int>::iterator t;
	std::vector<Window*>::iterator m;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
	//std::vector<Window*>::iterator m;
	//for(m = windows.begin(); m != windows.end(); ++m) {
		m = windows.begin();
		m+=(*t);
		if(!(*m)->Visible)
			continue;
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
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
	/*if((lastW != NULL) && (lastW->Visible)) {
		if((lastW->XPos <= x) && (lastW->YPos <= y)) { //Maybe we are on the window, let's check
			if((lastW->XPos+lastW->Width >= x) && (lastW->YPos+lastW->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = lastW->GetControl(x,y);
				if(ctrl != NULL) {
					lastW->SetFocused(ctrl);
					ctrl->OnMouseDown(x-lastW->XPos-ctrl->XPos,y-lastW->YPos-ctrl->YPos, Button, Mod);
					lastF = ctrl;
				}
				return;
			}	
		}
	}*/
	std::vector<int>::iterator t;
	std::vector<Window*>::iterator m;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
	//std::vector<Window*>::iterator m;
	//for(m = windows.begin(); m != windows.end(); ++m) {
		m = windows.begin();
		m+=(*t);
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
	/*for(unsigned int i = 0; i < windows.size(); i++) {
		Window * w = windows[i];
		if(!w->Visible)
			continue;
		Control *ctrl = NULL;
		int c = 0;
		do {
			ctrl = w->GetControl(c++);
			if(ctrl == NULL)
				break;
			ctrl->OnMouseUp(x-w->XPos-ctrl->XPos,y-w->YPos-ctrl->YPos, Button, Mod);
		} while(true);
	}*/
	int i = 0;
	std::vector<int>::iterator t;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		Window * w = windows[(*t)];
		if(w == NULL) {
			printf("DANGER WILL ROBINSON!! The Top Most Window is NULL\n");
			return;
		}
		if((x>=w->XPos) && (x <= (w->XPos+w->Width)) && (y>=w->YPos) && (y<=(w->YPos+w->Height))) {
			printf("Broadcasting MouseUp Event on Window %d of %d\n", i, topwin.size());
			Control * ctrl = NULL;
			int c = 0;
			do {
				ctrl = w->GetControl(c++);
				if(ctrl == NULL)
					break;
				ctrl->OnMouseUp(x-w->XPos-ctrl->XPos,y-w->YPos-ctrl->YPos, Button, Mod);
			} while(true);
			break;
		}
		i++;
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
	if(lastF)
		lastF->OnSpecialKeyPress(Key);
}
