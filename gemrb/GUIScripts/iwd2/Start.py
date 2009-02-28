import GemRB
from LoadScreen import *

StartWindow = 0
ProtocolWindow = 0
QuitWindow = 0
QuickLoadSlot = 0

def OnLoad():
	global StartWindow, QuickLoadSlot

	screen_width = GemRB.GetSystemVariable (SV_WIDTH)
	screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
	if screen_width == 1024:
		GemRB.LoadWindowFrame("STON10L", "STON10R", "STON10T", "STON10B")
	GemRB.LoadWindowPack("GUICONN", 800, 600)
#main window
	StartWindow = GemRB.LoadWindowObject(0)
	StartWindow.SetFrame ()
	ProtocolButton = StartWindow.GetControl(0x00)
	NewGameButton = StartWindow.GetControl(0x02)
	LoadGameButton = StartWindow.GetControl(0x07)
	QuickLoadButton = StartWindow.GetControl(0x03)
	JoinGameButton = StartWindow.GetControl(0x0B)
	OptionsButton = StartWindow.GetControl(0x08)
	QuitGameButton = StartWindow.GetControl(0x01)
	StartWindow.CreateLabel(0x0fff0000, 0,0,800,30, "REALMS2", "", 1)
	VersionLabel = StartWindow.GetControl(0x0fff0000)
	VersionLabel.SetText(GEMRB_VERSION)
	ProtocolButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	NewGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	LoadGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)

	GemRB.SetVar("PlayMode",2)
	GameCount=GemRB.GetSaveGameCount()

	#looking for the quicksave
	EnableQuickLoad = IE_GUI_BUTTON_DISABLED
	for ActPos in range(GameCount):
		Slotname = GemRB.GetSaveGameAttrib(5,ActPos)
		# quick save is 2
		if Slotname == 2:
			EnableQuickLoad = IE_GUI_BUTTON_ENABLED
			QuickLoadSlot = ActPos
			break

	QuickLoadButton.SetStatus(EnableQuickLoad)
	JoinGameButton.SetStatus(IE_GUI_BUTTON_DISABLED)
	OptionsButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	QuitGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText(15413)
	elif LastProtocol == 1:
		ProtocolButton.SetText(13967)
	elif LastProtocol == 2:
		ProtocolButton.SetText(13968)
	NewGameButton.SetText(13963)
	LoadGameButton.SetText(13729)
	QuickLoadButton.SetText(33508)
	JoinGameButton.SetText(13964)
	OptionsButton.SetText(13905)
	QuitGameButton.SetText(13731)
	QuitGameButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)
	NewGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "NewGamePress")
	QuitGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "QuitPress")
	ProtocolButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ProtocolPress")
	OptionsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	LoadGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "LoadPress")
	QuickLoadButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "QuickLoadPress")
	StartWindow.SetVisible(1)
	GemRB.LoadMusicPL("Theme.mus")
	return

def ProtocolPress():
	global StartWindow, ProtocolWindow
	#GemRB.UnloadWindow(StartWindow)
	StartWindow.SetVisible(0)
	ProtocolWindow = GemRB.LoadWindowObject(1)
	
	#Disabling Unused Buttons in this Window
	Button = ProtocolWindow.GetControl(2)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl(3)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl(9)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	
	SinglePlayerButton = ProtocolWindow.GetControl(10)
	SinglePlayerButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SinglePlayerButton.SetText(15413)
	
	IPXButton = ProtocolWindow.GetControl(0)
	IPXButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	IPXButton.SetText(13967)
	
	TCPIPButton = ProtocolWindow.GetControl(1)
	TCPIPButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	TCPIPButton.SetText(13968)
	
	SinglePlayerButton.SetVarAssoc("Last Protocol Used", 0)
	IPXButton.SetVarAssoc("Last Protocol Used", 1)
	TCPIPButton.SetVarAssoc("Last Protocol Used", 2)
	
	TextArea = ProtocolWindow.GetControl(7)
	TextArea.SetText(11316)
	
	DoneButton = ProtocolWindow.GetControl(6)
	DoneButton.SetText(11973)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ProtocolDonePress")
	
	ProtocolWindow.SetVisible(1)
	return
	
def ProtocolDonePress():
	global StartWindow, ProtocolWindow
	if ProtocolWindow:
		ProtocolWindow.Unload()
	
	ProtocolButton = StartWindow.GetControl(0x00)
	
	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText(15413)
	elif LastProtocol == 1:
		ProtocolButton.SetText(13967)
	elif LastProtocol == 2:
		ProtocolButton.SetText(13968)
	
	StartWindow.SetVisible(1)
	return
	
def LoadPress():
	global StartWindow

	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("GUILOAD")
	return

def QuickLoadPress():
	global StartWindow, QuickLoadSlot

	StartLoadScreen()
	GemRB.LoadGame(QuickLoadSlot) # load & start game
	GemRB.EnterGame()
	return

def OptionsPress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("Options")
	return
	
def QuitPress():
	global StartWindow, QuitWindow
	StartWindow.SetVisible(0)
	QuitWindow = GemRB.LoadWindowObject(22)
	CancelButton = QuitWindow.GetControl(2)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "QuitCancelPress")
	CancelButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)
	
	QuitButton = QuitWindow.GetControl(1)
	QuitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "QuitQuitPress")
	QuitButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)
	
	TextArea = QuitWindow.GetControl(0)
	CancelButton.SetText(13727)
	QuitButton.SetText(15417)
	TextArea.SetText(19532)
	QuitWindow.SetVisible(1)
	return
	
def NewGamePress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("SPParty")
	return	

def QuitCancelPress():
	global StartWindow, QuitWindow
	if QuitWindow:
		QuitWindow.Unload()
	StartWindow.SetVisible(1)
	return
	
def QuitQuitPress():
	GemRB.Quit()
	return
