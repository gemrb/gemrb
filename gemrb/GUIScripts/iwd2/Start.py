import GemRB

StartWindow = 0
ProtocolWindow = 0
QuitWindow = 0

def OnLoad():
	global StartWindow

	GemRB.LoadWindowPack("GUICONN")

#main window
	StartWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowSize(StartWindow, 800, 600)
	ProtocolButton = GemRB.GetControl(StartWindow, 0x00)
	NewGameButton = GemRB.GetControl(StartWindow, 0x02)
	LoadGameButton = GemRB.GetControl(StartWindow, 0x07)
	QuickLoadButton = GemRB.GetControl(StartWindow, 0x03)
	JoinGameButton = GemRB.GetControl(StartWindow, 0x0B)
	OptionsButton = GemRB.GetControl(StartWindow, 0x08)
	QuitGameButton = GemRB.GetControl(StartWindow, 0x01)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,0,800,30, "REALMS2", "", 1)
	VersionLabel = GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, VersionLabel, GEMRB_VERSION)
	GemRB.SetControlStatus(StartWindow, ProtocolButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, NewGameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, LoadGameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, QuickLoadButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, QuitGameButton, IE_GUI_BUTTON_ENABLED)
	LastProtocol = GemRB.GetVar("Last Protocol Used");
	if LastProtocol == 0:
		GemRB.SetText(StartWindow, ProtocolButton, 15413)
	elif LastProtocol == 1:
		GemRB.SetText(StartWindow, ProtocolButton, 13967)
	elif LastProtocol == 2:
		GemRB.SetText(StartWindow, ProtocolButton, 13968)
	GemRB.SetText(StartWindow, NewGameButton, 13963)
	GemRB.SetText(StartWindow, LoadGameButton, 13729)
	GemRB.SetText(StartWindow, QuickLoadButton, 33508)
	GemRB.SetText(StartWindow, JoinGameButton, 13964)
	GemRB.SetText(StartWindow, OptionsButton, 13905)
	GemRB.SetText(StartWindow, QuitGameButton, 13731)
	GemRB.SetEvent(StartWindow, QuitGameButton, IE_GUI_BUTTON_ON_PRESS, "QuitPress")
	GemRB.SetEvent(StartWindow, ProtocolButton, IE_GUI_BUTTON_ON_PRESS, "ProtocolPress")
	GemRB.SetEvent(StartWindow, OptionsButton, IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	GemRB.SetEvent(StartWindow, LoadGameButton, IE_GUI_BUTTON_ON_PRESS, "LoadPress")
	GemRB.SetVisible(StartWindow, 1)
	GemRB.LoadMusicPL("Theme")
	GemRB.StartPL()
	return

def ProtocolPress():
	global StartWindow, ProtocolWindow
	#GemRB.UnloadWindow(StartWindow)
	GemRB.SetVisible(StartWindow, 0)
	ProtocolWindow = GemRB.LoadWindow(1)
	
	#Disabling Unused Buttons in this Window
	Button = GemRB.GetControl(ProtocolWindow, 2)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = GemRB.GetControl(ProtocolWindow, 3)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = GemRB.GetControl(ProtocolWindow, 9)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	
	SinglePlayerButton = GemRB.GetControl(ProtocolWindow, 10)
	GemRB.SetButtonFlags(ProtocolWindow, SinglePlayerButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, SinglePlayerButton, 15413)
	
	IPXButton = GemRB.GetControl(ProtocolWindow, 0)
	GemRB.SetButtonFlags(ProtocolWindow, IPXButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, IPXButton, 13967)
	
	TCPIPButton = GemRB.GetControl(ProtocolWindow, 1)
	GemRB.SetButtonFlags(ProtocolWindow, TCPIPButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, TCPIPButton, 13968)
	
	GemRB.SetVarAssoc(ProtocolWindow, SinglePlayerButton, "Last Protocol Used", 0)
	GemRB.SetVarAssoc(ProtocolWindow, IPXButton, "Last Protocol Used", 1)
	GemRB.SetVarAssoc(ProtocolWindow, TCPIPButton, "Last Protocol Used", 2)
	
	TextArea = GemRB.GetControl(ProtocolWindow, 7)
	GemRB.SetText(ProtocolWindow, TextArea, 11316)
	
	DoneButton = GemRB.GetControl(ProtocolWindow, 6)
	GemRB.SetText(ProtocolWindow, DoneButton, 11973)
	GemRB.SetEvent(ProtocolWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "ProtocolDonePress")
	
	GemRB.SetVisible(ProtocolWindow, 1)
	return;
	
def ProtocolDonePress():
	global StartWindow, ProtocolWindow
	GemRB.UnloadWindow(ProtocolWindow)
	
	ProtocolButton = GemRB.GetControl(StartWindow, 0x00)
	
	LastProtocol = GemRB.GetVar("Last Protocol Used");
	if LastProtocol == 0:
		GemRB.SetText(StartWindow, ProtocolButton, 15413)
	elif LastProtocol == 1:
		GemRB.SetText(StartWindow, ProtocolButton, 13967)
	elif LastProtocol == 2:
		GemRB.SetText(StartWindow, ProtocolButton, 13968)
	
	GemRB.SetVisible(StartWindow, 1)
	return
	
def LoadPress():
	global StartWindow

	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("GUILOAD")
	return

def OptionsPress():
	global StartWindow
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("Options")
	return
	
def QuitPress():
	global StartWindow, QuitWindow
	GemRB.SetVisible(StartWindow, 0)
	QuitWindow = GemRB.LoadWindow(22);
	CancelButton = GemRB.GetControl(QuitWindow, 2)
	GemRB.SetEvent(QuitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "QuitCancelPress")
	
	QuitButton = GemRB.GetControl(QuitWindow, 1)
	GemRB.SetEvent(QuitWindow, QuitButton, IE_GUI_BUTTON_ON_PRESS, "QuitQuitPress")
	
	TextArea = GemRB.GetControl(QuitWindow, 0)
	GemRB.SetText(QuitWindow, CancelButton, 13727);
	GemRB.SetText(QuitWindow, QuitButton, 15417);
	GemRB.SetText(QuitWindow, TextArea, 19532);
	GemRB.SetVisible(QuitWindow, 1)
	return
	
def QuitCancelPress():
	global StartWindow, QuitWindow
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVisible(StartWindow, 1)
	return
	
def QuitQuitPress():
	GemRB.Quit()
	return
