import GemRB

MessageWindow = 0
MessageTextArea = 0

def OnLoad():
	global MessageWindow, MessageTextArea
	GemRB.LoadWindowPack("GUIW08")
	MessageWindow = GemRB.LoadWindow(22)
	MessageTextArea = GemRB.GetControl(MessageWindow, 1)
	#GemRB.SetText(MessageWindow, MessageTextArea, "")
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("MessageTextArea", MessageTextArea)
	
	GemRB.SetVisible(MessageWindow, 1)
	return