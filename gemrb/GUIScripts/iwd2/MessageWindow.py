import GemRB

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
	
	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("PortraitPosition", 4)
	
	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return

def PopulatePortraitWindow():
	Window = PortraitWindow

	for i in range (0,5):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "SelectedSingle", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		
		pic = GemRB.GetPlayerPortrait (i+1,1)
		GemRB.SetButtonPicture(Window, Button, pic)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE, OP_SET)
