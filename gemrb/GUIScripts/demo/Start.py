import GemRB
from GUIDefines import *

Name = ""
StartWindow = None

def OnLoad():
	global StartWindow

	# shortcircuting support for easier development
	if GemRB.GetVar ("SkipIntroVideos"):
		NewGamePress("Jora the Seeker")
		return

	StartWindow = GemRB.LoadWindow(0, "START")

	NewGameButton = StartWindow.GetControl(0)
	QuitGameButton = StartWindow.GetControl(1)
	NewGameButton.SetText("New demo")
	QuitGameButton.SetText("Quit")
	NewGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NewGamePress)
	QuitGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitPress)
	QuitGameButton.MakeEscape ()

	VersionLabel = StartWindow.GetControl(0x10000002)
	VersionLabel.SetText(GemRB.Version)

	NameEdit = StartWindow.GetControl(3)
	NameEdit.SetText("Jora the Seeker")
	NameEdit.Focus()

def QuitPress():
	global StartWindow

	if StartWindow:
		StartWindow.Unload()

	GemRB.Quit()

def NewGamePress(name = ""):
	global StartWindow, Name

	if name and isinstance(name, str):
		Name = name
	else:
		Name = StartWindow.GetControl(3).QueryText()

	if StartWindow:
		StartWindow.Unload()

	GemRB.LoadGame(None)
	# this is needed, so the game loop runs and the load happens
	# before other code (eg. CreatePlayer) depending on it is run
	GemRB.SetNextScript("SetupGame")
