#load window
import GemRB

LoadWindow = 0
TextAreaControl = 0
GameCount = 0

def OnLoad():
	global LoadWindow, TextAreaControl, GameCount

	GemRB.LoadWindowPack("GUILOAD")
	LoadWindow = GemRB.LoadWindow(0)
	CancelButton=GemRB.GetControl(LoadWindow,34)
	GemRB.SetText(LoadWindow, CancelButton, 13727)
	GemRB.SetEvent(LoadWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVar("LoadIdx",0)
	GemRB.SetVar("TopIndex",0)
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?

	for i in range(0,4):
		Button = GemRB.GetControl(LoadWindow,26+i)
		GemRB.SetText(LoadWindow, Button, 15590)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)

		Button = GemRB.GetControl(LoadWindow, 30+i)
		GemRB.SetText(LoadWindow, Button, 13957)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)

		#area previews
		Button = GemRB.GetControl(LoadWindow, 1+i)
		GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(0,6):
			Button = GemRB.GetControl(LoadWindow,40+i*6+j)
			GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=GemRB.GetControl(LoadWindow, 25)
	GemRB.SetVarAssoc(LoadWindow, ScrollBar, "TopIndex", GameCount)
	GemRB.SetEvent(LoadWindow, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	ScrollBarPress()
	GemRB.SetVisible(LoadWindow,1)
	return

def ScrollBarPress():
	#draw load game portraits
	Pos = GemRB.GetVar("TopIndex")
	for i in range(0,4):
		ActPos = Pos + i

		if ActPos<GameCount:
			Button = GemRB.GetControl(LoadWindow,26+i)
			GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_ENABLED)
			Button = GemRB.GetControl(LoadWindow, 30+i)
			GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_ENABLED)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(0,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x10000008+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(3,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x10000010+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		Button=GemRB.GetControl(LoadWindow, 1+i)
		if ActPos<GameCount:
			GemRB.SetSaveGamePreview(LoadWindow, Button, ActPos)
		else:
			GemRB.SetButtonPicture(LoadWindow, Button, "")
		for j in range(0,6):
			Button=GemRB.GetControl(LoadWindow, 40 + i*6 + j)
			if ActPos<GameCount:
				GemRB.SetSaveGamePortrait(LoadWindow, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture(LoadWindow, Button, "")
	return

def LoadGamePress():
	Pos = GemRB.GetVar("TopIndex")+GemRB.GetVar("LoadIdx")
	return

def DeleteGamePress():
	Pos = GemRB.GetVar("TopIndex")+GemRB.GetVar("LoadIdx")
	return
	
def CancelPress():
	GemRB.UnloadWindow(LoadWindow)
	GemRB.SetNextScript("Start")
	return
