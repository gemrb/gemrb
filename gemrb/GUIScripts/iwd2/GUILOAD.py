#load window
import GemRB

LoadWindow = 0
TextAreaControl = 0
GameCount = 0

def OnLoad():
	global LoadWindow, TextAreaControl, GameCount

	GemRB.SetVar("PlayMode",2)   #iwd2 is always using 'mpsave'
	GemRB.LoadWindowPack("GUILOAD")
	LoadWindow = GemRB.LoadWindow(0)
	CancelButton=GemRB.GetControl(LoadWindow,22)
	GemRB.SetText(LoadWindow, CancelButton, 13727)
	GemRB.SetEvent(LoadWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVar("LoadIdx",0)
	GemRB.SetVar("TopIndex",0)
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?

	for i in range(0,5):
		Button = GemRB.GetControl(LoadWindow,55+i)
		GemRB.SetText(LoadWindow, Button, 15590)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)

		Button = GemRB.GetControl(LoadWindow, 60+i)
		GemRB.SetText(LoadWindow, Button, 13957)
		GemRB.SetEvent(LoadWindow, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(LoadWindow, Button, "LoadIdx",i)

		#area previews
		Button = GemRB.GetControl(LoadWindow, 1+i)
		GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(0,6):
			Button = GemRB.GetControl(LoadWindow,25+i*6+j)
			GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
			GemRB.SetControlSize(LoadWindow, Button, 21, 21)

	ScrollBar=GemRB.GetControl(LoadWindow, 23)
	GemRB.SetVarAssoc(LoadWindow, ScrollBar, "TopIndex", GameCount)
	GemRB.SetEvent(LoadWindow, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	ScrollBarPress()
	GemRB.SetVisible(LoadWindow,1)
	return

def ScrollBarPress():
	#draw load game portraits
	Pos = GemRB.GetVar("TopIndex")
	for i in range(0,5):
		ActPos = Pos + i

		Button1 = GemRB.GetControl(LoadWindow,55+i)
		Button2 = GemRB.GetControl(LoadWindow, 60+i)
		if ActPos<GameCount:
			GemRB.SetButtonState(LoadWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(LoadWindow, Button2, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState(LoadWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(LoadWindow, Button2, IE_GUI_BUTTON_DISABLED)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(0,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x10000005+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(3,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x1000000a+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(3,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x1000000f+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		Button=GemRB.GetControl(LoadWindow, 1+i)
		if ActPos<GameCount:
			GemRB.SetSaveGamePreview(LoadWindow, Button, ActPos)
		else:
			GemRB.SetButtonPicture(LoadWindow, Button, "")
		for j in range(0,6):
			Button=GemRB.GetControl(LoadWindow, 25 + i*6 + j)
			if ActPos<GameCount:
				GemRB.SetSaveGamePortrait(LoadWindow, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture(LoadWindow, Button, "")
	return

def LoadGamePress():
	Pos = GemRB.GetVar("TopIndex")+GemRB.GetVar("LoadIdx")
	return

def DeleteGameConfirm():
	TopIndex = GemRB.GetVar("TopIndex")
	Pos = TopIndex +GemRB.GetVar("LoadIdx")
	GemRB.DeleteSaveGame(Pos)
	if TopIndex>0:
		GemRB.SetVar("TopIndex",TopIndex-1)
	ScrollBarPress()
	GemRB.UnloadWindow(ConfirmWindow)
	GemRB.SetVisible(LoadWindow,1)
	return          
                
def DeleteGameCancel(): 
	GemRB.UnloadWindow(ConfirmWindow)
	GemRB.SetVisible(LoadWindow,1)
	return

def DeleteGamePress():
	global ConfirmWindow

	ConfirmWindow=GemRB.LoadWindow(1)
	DeleteButton=GemRB.GetControl(ConfirmWindow, 1)
	GemRB.SetText(ConfirmWindow, DeleteButton, 13575)
	GemRB.SetEvent(ConfirmWindow, DeleteButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameConfirm")
	CancelButton=GemRB.GetControl(ConfirmWindow, 2)
	GemRB.SetText(ConfirmWindow, CancelButton, 13727)
	GemRB.SetEvent(ConfirmWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameCancel")
	GemRB.SetVisible(ConfirmWindow,1)
	return
	
def CancelPress():
	GemRB.UnloadWindow(LoadWindow)
	GemRB.SetNextScript("Start")
	return
