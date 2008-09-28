#load window
import GemRB
from LoadScreen import *

LoadWindow = 0
TextAreaControl = 0
GameCount = 0
ScrollBar = 0

def OnLoad():
	global LoadWindow, TextAreaControl, GameCount, ScrollBar

	if GemRB.GetVar("oldgame")==0:
		GemRB.GameSetExpansion(1)
	else:
		GemRB.GameSetExpansion(0)
	
	GemRB.LoadWindowPack("GUILOAD", 640, 480)
	LoadWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame (LoadWindow)
	CancelButton=GemRB.GetControl(LoadWindow,34)
	GemRB.SetText(LoadWindow, CancelButton, 13727)
	GemRB.SetEvent(LoadWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVar("LoadIdx",0)

	for i in range(4):
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
		GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(PARTY_SIZE):
			Button = GemRB.GetControl(LoadWindow,40+i*PARTY_SIZE+j)
			GemRB.SetButtonState(LoadWindow, Button, IE_GUI_BUTTON_LOCKED)
			GemRB.SetButtonFlags(LoadWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=GemRB.GetControl(LoadWindow, 25)
	GemRB.SetEvent(LoadWindow, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?
	if GameCount>4:
		TopIndex = GameCount-4
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex",TopIndex)
	GemRB.SetVarAssoc (LoadWindow, ScrollBar, "TopIndex", TopIndex+1)
	ScrollBarPress ()
	GemRB.SetVisible (LoadWindow,1)
	return

def ScrollBarPress():
	#draw load game portraits
	Pos = GemRB.GetVar("TopIndex")
	for i in range(4):
		ActPos = Pos + i

		Button1 = GemRB.GetControl(LoadWindow,26+i)
		Button2 = GemRB.GetControl(LoadWindow, 30+i)
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
		Label = GemRB.GetControl(LoadWindow, 0x10000008+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(4,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(LoadWindow, 0x10000010+i)
		GemRB.SetText(LoadWindow, Label, Slotname)

		Button=GemRB.GetControl(LoadWindow, 1+i)
		if ActPos<GameCount:
			GemRB.SetSaveGamePreview(LoadWindow, Button, ActPos)
		else:
			GemRB.SetButtonPicture(LoadWindow, Button, "")
		for j in range(PARTY_SIZE):
			Button=GemRB.GetControl(LoadWindow, 40+i*PARTY_SIZE+j)
			if ActPos<GameCount:
				GemRB.SetSaveGamePortrait(LoadWindow, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture(LoadWindow, Button, "")
	return

def LoadGamePress():
	GemRB.UnloadWindow(LoadWindow)
	Pos = GemRB.GetVar("TopIndex")+GemRB.GetVar("LoadIdx")
	StartLoadScreen()
	GemRB.LoadGame(Pos) #loads and enters savegame 
	GemRB.EnterGame() #it will close windows, including the loadscreen
	return

def DeleteGameConfirm():
	global GameCount

	TopIndex = GemRB.GetVar("TopIndex")
	Pos = TopIndex +GemRB.GetVar("LoadIdx")
	GemRB.DeleteSaveGame(Pos)
	if TopIndex>0:
		GemRB.SetVar("TopIndex",TopIndex-1)
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?
	GemRB.SetVarAssoc(LoadWindow, ScrollBar, "TopIndex", GameCount)
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

	GemRB.SetVisible(LoadWindow, 0)
	ConfirmWindow=GemRB.LoadWindow(1)
	Text=GemRB.GetControl(ConfirmWindow, 0)
	GemRB.SetText(ConfirmWindow, Text, 15305)
	DeleteButton=GemRB.GetControl(ConfirmWindow, 1)
	GemRB.SetText(ConfirmWindow, DeleteButton, 13957)
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
