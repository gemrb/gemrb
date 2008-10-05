#load window
import GemRB
from LoadScreen import *
from GUICommon import CloseOtherWindow

SaveWindow = 0
ConfirmWindow = 0
NameField = 0
SaveButton = 0
TextAreaControl = 0
GameCount = 0
ScrollBar = 0

def OpenSaveWindow ():
	global SaveWindow, TextAreaControl, GameCount, ScrollBar

	if CloseOtherWindow (OpenSaveWindow):
		CloseSaveWindow ()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUISAVE", 800, 600)
	Window = SaveWindow = GemRB.LoadWindow (0)
	GemRB.SetWindowFrame (Window)
	CancelButton=GemRB.GetControl (Window,22)
	GemRB.SetText (Window, CancelButton, 13727)
	GemRB.SetEvent (Window,CancelButton,IE_GUI_BUTTON_ON_PRESS, "OpenSaveWindow")
	GemRB.SetVar ("LoadIdx",0)

	for i in range(5):
		Button = GemRB.GetControl (Window,55+i)
		GemRB.SetText (Window, Button, 15588)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SavePress")
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc (Window, Button, "LoadIdx",i)

		Button = GemRB.GetControl (Window, 60+i)
		GemRB.SetText (Window, Button, 13957)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc (Window, Button, "LoadIdx",i)

		#area previews
		Button = GemRB.GetControl (Window, 1+i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(PARTY_SIZE):
			Button = GemRB.GetControl (Window,25+i*PARTY_SIZE+j)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
			GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=GemRB.GetControl (Window, 23)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GameCount=GemRB.GetSaveGameCount ()+1 #one more for the 'new game'
	if GameCount>4:
		TopIndex = GameCount-4
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex",TopIndex)
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", TopIndex+1)
	GemRB.SetDefaultScrollBar (Window, ScrollBar)
	ScrollBarPress ()
	GemRB.SetVisible (Window,1)
	return

def ScrollBarPress():
	Window = SaveWindow

	#draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range(5):
		ActPos = Pos + i

		Button1 = GemRB.GetControl (Window,55+i)
		Button2 = GemRB.GetControl (Window, 60+i)
		if ActPos<GameCount:
			GemRB.SetButtonState (Window, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState (Window, Button2, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState (Window, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState (Window, Button2, IE_GUI_BUTTON_DISABLED)

		if ActPos<GameCount-1:
			Slotname = GemRB.GetSaveGameAttrib (0,ActPos)
		elif ActPos == GameCount-1:
			Slotname = 15304
		else:
			Slotname = ""
		Label = GemRB.GetControl (Window, 0x10000005+i)
		GemRB.SetText (Window, Label, Slotname)

		if ActPos<GameCount-1:
			Slotname = GemRB.GetSaveGameAttrib (4,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl (Window, 0x1000000a+i)
		GemRB.SetText (Window, Label, Slotname)

		Button=GemRB.GetControl (Window, 1+i)
		if ActPos<GameCount-1:
			GemRB.SetSaveGamePreview(Window, Button, ActPos)
		else:
			GemRB.SetButtonPicture(Window, Button, "")
		for j in range(PARTY_SIZE):
			Button=GemRB.GetControl (Window, 25+i*PARTY_SIZE+j)
			if ActPos<GameCount-1:
				GemRB.SetSaveGamePortrait(Window, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture(Window, Button, "")
	return

def AbortedSaveGame():
	GemRB.UnloadWindow (ConfirmWindow)
	GemRB.SetVisible (SaveWindow,1)
	return

def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	Label = GemRB.GetControl (ConfirmWindow, 3)
	Slotname = GemRB.QueryText (ConfirmWindow, Label)
	StartLoadScreen()
	GemRB.SaveGame(Pos, Slotname, 22) #saves a game with version 2.2
	GemRB.UnloadWindow (ConfirmWindow)
	GemRB.SetVisible (SaveWindow,1)
	return

def SavePress():
	global ConfirmWindow, NameField, SaveButton

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	ConfirmWindow = GemRB.LoadWindow (1)

	#slot name
	if Pos<GameCount-1:
		Slotname = GemRB.GetSaveGameAttrib (0,Pos)
	else:
		Slotname = ""
	NameField = GemRB.GetControl (ConfirmWindow, 3)
	GemRB.SetText (ConfirmWindow, NameField, Slotname)
	GemRB.SetEvent (ConfirmWindow, NameField, IE_GUI_EDIT_ON_CHANGE,"EditChange")

	#game hours (should be generated from game)
	if Pos<GameCount-1:
		Slotname = GemRB.GetSaveGameAttrib (4,Pos)
	else:
		Slotname = ""
	Label = GemRB.GetControl (ConfirmWindow, 0x10000004)
	GemRB.SetText (ConfirmWindow, Label, Slotname)

	#areapreview
	#Button=GemRB.GetControl (ConfirmWindow, 0)
	#if Pos<GameCount-1:
	#	GemRB.SetSaveGamePreview(ConfirmWindow, Button, Pos)
	#else:
	#	GemRB.SetButtonPicture(ConfirmWindow, Button, "")

	#portraits
	#for j in range(PARTY_SIZE):
	#	Button=GemRB.GetControl (ConfirmWindow, 25+j)
	#	if Pos<GameCount-1:
	#		GemRB.SetSaveGamePortrait(ConfirmWindow, Button, Pos,j)
	#	else:
	#		GemRB.SetButtonPicture(ConfirmWindow, Button, "")

	#save
	SaveButton=GemRB.GetControl (ConfirmWindow, 7)
	GemRB.SetText (ConfirmWindow, SaveButton, 15588)
	GemRB.SetEvent (ConfirmWindow, SaveButton, IE_GUI_BUTTON_ON_PRESS, "ConfirmedSaveGame")
	GemRB.SetButtonFlags (ConfirmWindow, SaveButton, IE_GUI_BUTTON_DEFAULT, OP_OR)
	#GemRB.SetButtonState (ConfirmWindow, SaveButton, IE_GUI_BUTTON_DISABLED)
	if Slotname == "":
		GemRB.SetButtonState (ConfirmWindow, SaveButton, IE_GUI_BUTTON_DISABLED)

	#cancel
	CancelButton=GemRB.GetControl (ConfirmWindow, 8)
	GemRB.SetText (ConfirmWindow, CancelButton, 13727)
	GemRB.SetEvent (ConfirmWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "AbortedSaveGame")
	GemRB.SetVisible (ConfirmWindow,1)
	GemRB.SetControlStatus (ConfirmWindow, NameField, IE_GUI_CONTROL_FOCUSED)
	return

def EditChange():
	Name = GemRB.QueryText (ConfirmWindow, NameField)
	if len(Name)==0:
		GemRB.SetButtonState (ConfirmWindow, SaveButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (ConfirmWindow, SaveButton, IE_GUI_BUTTON_ENABLED)
	return

def DeleteGameConfirm():
	global GameCount

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Pos)
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	GameCount=GemRB.GetSaveGameCount () #count of games in save folder?
	GemRB.SetVarAssoc (SaveWindow, ScrollBar, "TopIndex", GameCount)
	ScrollBarPress()
	GemRB.UnloadWindow (ConfirmWindow)
	GemRB.SetVisible (SaveWindow,1)
	return

def DeleteGameCancel():
	GemRB.UnloadWindow (ConfirmWindow)
	GemRB.SetVisible (SaveWindow,1)
	return

def DeleteGamePress():
	global ConfirmWindow

	GemRB.SetVisible (SaveWindow, 0)
	ConfirmWindow=GemRB.LoadWindow (2)
	Text=GemRB.GetControl (ConfirmWindow, 0)
	GemRB.SetText (ConfirmWindow, Text, 15305)
	DeleteButton=GemRB.GetControl (ConfirmWindow, 1)
	GemRB.SetText (ConfirmWindow, DeleteButton, 13957)
	GemRB.SetEvent (ConfirmWindow, DeleteButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameConfirm")
	CancelButton=GemRB.GetControl (ConfirmWindow, 2)
	GemRB.SetText (ConfirmWindow, CancelButton, 13727)
	GemRB.SetEvent (ConfirmWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameCancel")
	GemRB.SetVisible (ConfirmWindow,1)
	return

def CloseSaveWindow ():
	GemRB.UnloadWindow (SaveWindow)
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GemRB.SetVisible (0,1) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	return
