import GemRB

PictureWindow = 0

def OnLoad():
	global PictureWindow
	GemRB.LoadWindowPack("GUIMG")
	TimeWindow = GemRB.LoadWindow(0)
	PictureWindow = GemRB.LoadWindow(1)
	ButtonWindow = GemRB.LoadWindow(2)
	GemRB.SetVar("MessageWindow", PictureWindow)		
	
	GemRB.SetVisible(TimeWindow, 1)
	GemRB.SetVisible(PictureWindow, 1)
	GemRB.SetVisible(ButtonWindow, 1)
	return