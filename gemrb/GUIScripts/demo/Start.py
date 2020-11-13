import GemRB

StartWindow = 0

def OnLoad():
	global StartWindow

	GemRB.LoadGame(None)

	GemRB.LoadWindowPack("GUICONN", 800, 600)

	StartWindow = GemRB.LoadWindow(0)

	NewGameButton = StartWindow.GetControl(0x00)

	QuitGameButton = StartWindow.GetControl(0x01)

	NewGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)

	QuitGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)

	NewGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NewGamePress)
	QuitGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitPress)

	StartWindow.SetVisible(WINDOW_VISIBLE)

	return

def QuitPress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.QuitGame()
	GemRB.Quit()
	return

def NewGamePress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("SetupGame")
	return
