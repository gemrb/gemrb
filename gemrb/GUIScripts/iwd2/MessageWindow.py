import GemRB

from GUICommonWindows import *

MessageWindow = 0
MessageTA = 0

def OnLoad():
	global MessageWindow, MessageTA
	GemRB.LoadWindowPack("GUIW08")
	MessageWindow = GemRB.LoadWindow(22)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)

	PortraitWindow = GemRB.LoadWindow(1)
	
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("MessageTextArea", MessageTA)
	
	PopulatePortraitWindow(PortraitWindow)

	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("PortraitPosition", 4)
	
	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return
