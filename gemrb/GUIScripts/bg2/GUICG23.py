#character generation, biography (GUICG23)
import GemRB

ImportWindow = 0

def OnLoad():
	global ImportWindow

	GemRB.LoadWindowPack("GUICG")
	ImportWindow = GemRB.LoadWindow(23)

	TextAreaControl = GemRB.GetControl(ImportWindow, 0)
	GemRB.SetText(ImportWindow, TextAreaControl, 53605)

	FileButton = GemRB.GetControl(ImportWindow, 1)
	GemRB.SetText(ImportWindow, FileButton, 53604)

	SavedGameButton = GemRB.GetControl(ImportWindow,2)
	GemRB.SetText(ImportWindow, SavedGameButton, 53602)

	CancelButton = GemRB.GetControl(ImportWindow,3)
	GemRB.SetText(ImportWindow, CancelButton, 13727)

	GemRB.SetEvent(ImportWindow, FileButton, IE_GUI_BUTTON_ON_PRESS, "FilePress")
	GemRB.SetEvent(ImportWindow, SavedGameButton, IE_GUI_BUTTON_ON_PRESS, "GamePress")
	GemRB.SetEvent(ImportWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(ImportWindow,1)
	return

def FilePress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("ImportFile")
	return
	
def GamePress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("ImportGame")
	return

def CancelPress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("CharGen")
	return
