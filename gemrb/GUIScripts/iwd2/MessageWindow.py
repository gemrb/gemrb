import GemRB

MessageWindow = 0
MessageTextArea = 0

def OnLoad():
	global MessageWindow, MessageTextArea
	GemRB.LoadWindowPack("GUIW08")
	MessageWindow = GemRB.LoadWindow(22)
	MessageTextArea = GemRB.GetControl(MessageWindow, 1)
	PortraitWindow = GemRB.LoadWindow(1)
	
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("MessageTextArea", MessageTextArea)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	
	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("PortraitPosition", 4)
	
	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return