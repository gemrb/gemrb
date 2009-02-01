#character generation, import (GUICG24)
import GemRB

ImportWindow = 0

def OnLoad():
	global ImportWindow

	GemRB.LoadWindowPack("GUICG", 640, 480)
	ImportWindow = GemRB.LoadWindowObject(24)

	TextAreaControl = ImportWindow.GetControl(0)
	TextAreaControl.SetText(53605)

	FileButton = ImportWindow.GetControl(1)
	FileButton.SetText(53604)

	SavedGameButton = ImportWindow.GetControl(2)
	SavedGameButton.SetText(53602)

	CancelButton = ImportWindow.GetControl(3)
	CancelButton.SetText(13727)

	FileButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "FilePress")
	SavedGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GamePress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	ImportWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def FilePress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("ImportFile")
	return
	
def GamePress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("ImportGame")
	return

def CancelPress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("CharGen")
	return
