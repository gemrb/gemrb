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
	print "GameCount", GameCount

	for i in range(0,4):
		Button = GemRB.GetControl(LoadWindow,26+i)
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetText(LoadWindow, Button, 15590)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")

		Button = GemRB.GetControl(LoadWindow, 30+i)
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetText(LoadWindow, Button, 13957)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")

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

		Slotname = GemRB.GetSaveGameAttrib(0,ActPos)
		Label = GemRB.GetControl(LoadWindow, 0x10000008+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		Slotname = GemRB.GetSaveGameAttrib(1,ActPos)
		Label = GemRB.GetControl(LoadWindow, 0x10000010+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		Button=GemRB.GetControl(LoadWindow, 1+i)
		if ActPos<GameCount:
		#			Portrait=GemRB.GetAreaPortrait(ActPos)
			Portrait=""
		else:
			Portrait=""
		GemRB.SetButtonPicture(LoadWindow, Button, Portrait)
		for j in range(0,8):
			Button=GemRB.GetControl(LoadWindow, 40 + i*6 + j)
			if ActPos<GameCount:
		#				Portrait=GemRB.GetPCPortrait(ActPos,j+1)
				Portrait=""
			GemRB.SetButtonPicture(LoadWindow, Button, Portrait)
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
