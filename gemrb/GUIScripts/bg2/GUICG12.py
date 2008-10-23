#character generation, appearance (GUICG12)
import GemRB

AppearanceWindow = 0
CustomWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture ():
	global PortraitsTable, LastPortrait

	PortraitName = GemRB.GetTableRowName (PortraitsTable, LastPortrait)+"L"
	GemRB.SetButtonPicture (AppearanceWindow, PortraitButton, PortraitName, "NOPORTLG")
	return

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender
	
	Gender=GemRB.GetVar ("Gender")

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	AppearanceWindow = GemRB.LoadWindow (11)

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTable ("PICTURES")
	LastPortrait = 0

	TextAreaControl = GemRB.GetControl (AppearanceWindow, 7)
	GemRB.SetText (AppearanceWindow, TextAreaControl,"") # why is this here?

	PortraitButton = GemRB.GetControl (AppearanceWindow, 1)
	GemRB.SetButtonFlags (AppearanceWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	GemRB.SetButtonState (AppearanceWindow, PortraitButton, IE_GUI_BUTTON_LOCKED)

	LeftButton = GemRB.GetControl (AppearanceWindow,2)
	RightButton = GemRB.GetControl (AppearanceWindow,3)

	BackButton = GemRB.GetControl (AppearanceWindow,5)
	GemRB.SetText (AppearanceWindow,BackButton,15416)

	CustomButton = GemRB.GetControl (AppearanceWindow, 6)
	GemRB.SetText (AppearanceWindow, CustomButton, 17545)

	DoneButton = GemRB.GetControl (AppearanceWindow,0)
	GemRB.SetText (AppearanceWindow,DoneButton,11973)
	GemRB.SetButtonFlags (AppearanceWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent (AppearanceWindow,RightButton,IE_GUI_BUTTON_ON_PRESS,"RightPress")
	GemRB.SetEvent (AppearanceWindow,LeftButton,IE_GUI_BUTTON_ON_PRESS,"LeftPress")
	GemRB.SetEvent (AppearanceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent (AppearanceWindow,CustomButton,IE_GUI_BUTTON_ON_PRESS,"CustomPress")
	GemRB.SetEvent (AppearanceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	
	while True:
		if GemRB.GetTableValue (PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture ()
			break
		LastPortrait = LastPortrait + 1
	GemRB.SetVisible (AppearanceWindow,1)
	return

def RightPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait + 1
		if LastPortrait >= GemRB.GetTableRowCount (PortraitsTable):
			LastPortrait = 0
		if GemRB.GetTableValue (PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture ()
			return

def LeftPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait - 1
		if LastPortrait < 0:
			LastPortrait = GemRB.GetTableRowCount (PortraitsTable)-1
		if GemRB.GetTableValue (PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture ()
			return

def BackPress():
	GemRB.UnloadWindow (AppearanceWindow)
	GemRB.SetNextScript ("GUICG1")
	GemRB.SetVar ("Gender",0) #scrapping the gender value
	return

def CustomDone():
	Window = CustomWindow

	Portrait = GemRB.QueryText (Window, PortraitList1)
	GemRB.SetToken ("LargePortrait", Portrait)
	Portrait = GemRB.QueryText (Window, PortraitList2)
	GemRB.SetToken ("SmallPortrait", Portrait)
	GemRB.UnloadWindow (Window)
	GemRB.UnloadWindow (AppearanceWindow)
	GemRB.SetNextScript ("CharGen2")
	return

def CustomAbort():
	GemRB.UnloadWindow (CustomWindow)
	return

def LargeCustomPortrait():
	Window = CustomWindow

	Portrait = GemRB.QueryText (Window, PortraitList1)
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = GemRB.GetControl (Window, 0x10000007)
	GemRB.SetText (Window, Label, Portrait)

	Button = GemRB.GetControl (Window, 6)
	if Portrait=="":
		Portrait = "NOPORTMD"
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		if GemRB.QueryText (Window, PortraitList2)!="":
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	Button = GemRB.GetControl (Window, 0)
	GemRB.SetButtonPicture (Window, Button, Portrait, "NOPORTMD")
	return

def SmallCustomPortrait():
	Window = CustomWindow

	Portrait = GemRB.QueryText (Window, PortraitList2)
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = GemRB.GetControl (Window, 0x10000008)
	GemRB.SetText (Window, Label, Portrait)

	Button = GemRB.GetControl (Window, 6)
	if Portrait=="":
		Portrait = "NOPORTSM"
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		if GemRB.QueryText (Window, PortraitList1)!="":
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetButtonPicture (Window, Button, Portrait, "NOPORTSM")
	return

def CustomPress():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow (18)
	PortraitList1 = GemRB.GetControl (Window, 2)
	RowCount1 = GemRB.GetPortraits (Window, PortraitList1, 0)
	GemRB.SetEvent (Window, PortraitList1, IE_GUI_TEXTAREA_ON_CHANGE, "LargeCustomPortrait")
	GemRB.SetVar ("Row1", RowCount1)
	GemRB.SetVarAssoc (Window, PortraitList1, "Row1",RowCount1)

	PortraitList2 = GemRB.GetControl (Window, 4)
	RowCount2 = GemRB.GetPortraits (Window, PortraitList2, 1)
	GemRB.SetEvent (Window, PortraitList2, IE_GUI_TEXTAREA_ON_CHANGE, "SmallCustomPortrait")
	GemRB.SetVar ("Row2", RowCount2)
	GemRB.SetVarAssoc (Window, PortraitList2, "Row2",RowCount2)

	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CustomDone")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 15416)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CustomAbort")

	Button = GemRB.GetControl (Window, 0)
	PortraitName = GemRB.GetTableRowName (PortraitsTable, LastPortrait)+"M"
	GemRB.SetButtonPicture (Window, Button, PortraitName, "NOPORTMD")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	Button = GemRB.GetControl (Window, 1)
	PortraitName = GemRB.GetTableRowName (PortraitsTable, LastPortrait)+"S"
	GemRB.SetButtonPicture (Window, Button, PortraitName, "NOPORTSM")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	GemRB.ShowModal (Window, MODAL_SHADOW_NONE)
	return

def NextPress():
	GemRB.UnloadWindow (AppearanceWindow)
	PortraitTable = GemRB.LoadTable ("pictures")
	PortraitName = GemRB.GetTableRowName (PortraitTable, LastPortrait )
	GemRB.SetToken ("SmallPortrait", PortraitName+"S")
	GemRB.SetToken ("LargePortrait", PortraitName+"M")
	GemRB.UnloadTable (PortraitTable)
	GemRB.SetNextScript ("CharGen2") #Before race
	return

