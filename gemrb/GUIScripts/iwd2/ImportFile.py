#character generation, import (GUICG20)
import GemRB

#import from a character sheet
MainWindow = 0
PortraitButton = 0
ImportWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global MainWindow, PortraitButton
	global ImportWindow, TextAreaControl, DoneButton

	GemRB.LoadWindowPack("GUICG", 800, 600)
	MainWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame(MainWindow)

	PortraitButton = GemRB.GetControl (MainWindow, 12)
	GemRB.SetButtonFlags(MainWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	ImportWindow = GemRB.LoadWindow(20)

	TextAreaControl = GemRB.GetControl(ImportWindow, 4)
	GemRB.SetText(ImportWindow, TextAreaControl, 10963)

	TextAreaControl = GemRB.GetControl(ImportWindow,2)
	GemRB.SetTextAreaFlags (ImportWindow, TextAreaControl, IE_GUI_TEXTAREA_SELECTABLE)
	GemRB.GetCharacters(ImportWindow, TextAreaControl)

	DoneButton = GemRB.GetControl(ImportWindow, 0)
	GemRB.SetText(ImportWindow, DoneButton, 36789)
	GemRB.SetButtonState(ImportWindow, DoneButton, IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(ImportWindow,1)
	GemRB.SetText(ImportWindow, CancelButton, 15416)
	
	# disable the three extraneous buttons in the bottom row
	for i in [16, 13, 15]:
		TmpButton = GemRB.GetControl(MainWindow, i)
		GemRB.SetButtonState(MainWindow, TmpButton, IE_GUI_BUTTON_DISABLED)

	GemRB.SetEvent(ImportWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetEvent(ImportWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(ImportWindow, TextAreaControl, IE_GUI_TEXTAREA_ON_CHANGE, "SelectFile")
	GemRB.SetVisible(MainWindow,1)
	GemRB.SetVisible(ImportWindow,1)
	return

def DonePress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.UnloadWindow(MainWindow)
	#this part is fuzzy
	#we don't have the character as an object in the chargen
	#but we just imported a complete object
	#either we take the important stats and destroy the object
	#or start with an object from the beginning
	#or use a different script here???
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.UnloadWindow(MainWindow)
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return

def SelectFile():
	FileName = GemRB.QueryText(ImportWindow, TextAreaControl)
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	Portrait = GemRB.GetPlayerPortrait (Slot,0)
	GemRB.SetButtonPicture (MainWindow, PortraitButton, Portrait, "NOPORTLG") 
	GemRB.SetVisible(ImportWindow,3) #bring it to the front
	GemRB.SetButtonState(ImportWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return
