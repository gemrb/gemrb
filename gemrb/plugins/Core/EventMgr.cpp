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
	if(win != NULL)
		windows.push_back(win);
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	std::vector<Window*>::iterator m;
	while(windows.size() != 0) {
		m = windows.begin();
		(*m)->release();
		windows.erase(m);
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	if(lastW != NULL) {
		if((lastW->XPos <= x) && (lastW->YPos <= y)) { //Maybe we are on the window, let's check
			if((lastW->XPos+lastW->Width >= x) && (lastW->YPos+lastW->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = lastW->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
						ctrl->OnMouseOver(x,y);
				}
				return;
			}	
		}
	}
	std::vector<Window*>::iterator m;
	for(m = windows.begin(); m != windows.end(); m++) {
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
						ctrl->OnMouseOver(x,y);
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
	if(lastW != NULL) {
		if((lastW->XPos <= x) && (lastW->YPos <= y)) { //Maybe we are on the window, let's check
			if((lastW->XPos+lastW->Width >= x) && (lastW->YPos+lastW->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = lastW->GetControl(x,y);
				if(ctrl != NULL) {
					lastW->SetFocused(ctrl);
					ctrl->OnMouseDown(x,y, Button, Mod);
					lastF = ctrl;
				}
				return;
			}	
		}
	}
	std::vector<Window*>::iterator m;
	for(m = windows.begin(); m != windows.end(); m++) {
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(ctrl != NULL) {
					(*m)->SetFocused(ctrl);
					ctrl->OnMouseDown(x,y, Button, Mod);
					lastF = ctrl;
				}
				lastW = *m;
				return;
			}	
		}
	}
	lastF->hasFocus = false;
	lastF = NULL;
}
/** BroadCast Mouse Move Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	if(lastW != NULL) {
		if((lastW->XPos <= x) && (lastW->YPos <= y)) { //Maybe we are on the window, let's check
			if((lastW->XPos+lastW->Width >= x) && (lastW->YPos+lastW->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = lastW->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
						ctrl->OnMouseUp(x,y, Button, Mod);
				}
				return;
			}	
		}
	}
	std::vector<Window*>::iterator m;
	for(m = windows.begin(); m != windows.end(); m++) {
		if(((*m)->XPos <= x) && ((*m)->YPos <= y)) { //Maybe we are on the window, let's check
			if(((*m)->XPos+(*m)->Width >= x) && ((*m)->YPos+(*m)->Height >= y)) { //Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control * ctrl = (*m)->GetControl(x,y);
				if(ctrl != NULL) {
					if(ctrl->hasFocus)
						ctrl->OnMouseUp(x,y, Button, Mod);
				}
				lastW = *m;
				return;
			}	
		}
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
