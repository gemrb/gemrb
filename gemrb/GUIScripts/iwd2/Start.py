import GemRB

StartWindow = 0

def OnLoad():
	global StartWindow

	GemRB.LoadWindowPack("START")

#main window
	StartWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowSize(StartWindow, 800, 600);
	GemRB.CreateButton(StartWindow, 0, 566, 160, 156, 30);
	GemRB.CreateButton(StartWindow, 1, 566, 248, 156, 30);
	GemRB.CreateButton(StartWindow, 2, 566, 280, 156, 30);
	GemRB.CreateButton(StartWindow, 3, 566, 312, 156, 30);
	GemRB.CreateButton(StartWindow, 4, 566, 344, 156, 30);
	GemRB.CreateButton(StartWindow, 5, 566, 396, 156, 30);
	GemRB.CreateButton(StartWindow, 6, 566, 428, 156, 30);
	GemRB.CreateLabel(StartWindow, 7, 567, 130, 154, 24, "NORMAL", "", 1);
	GemRB.CreateLabel(StartWindow, 8, 567, 218, 154, 24, "NORMAL", "", 1);
	ProtocolButton = GemRB.GetControl(StartWindow, 0)
	NewGameButton = GemRB.GetControl(StartWindow, 1);
	LoadGameButton = GemRB.GetControl(StartWindow, 2);
	QuickLoadButton = GemRB.GetControl(StartWindow, 3);
	JoinGameButton = GemRB.GetControl(StartWindow, 4);
	OptionsButton = GemRB.GetControl(StartWindow, 5);
	QuitGameButton = GemRB.GetControl(StartWindow, 6);
	GameModeLabel = GemRB.GetControl(StartWindow, 7);
	BeginGameLabel = GemRB.GetControl(StartWindow, 8);
	GemRB.SetLabelTextColor(StartWindow, GameModeLabel, 0xff, 0xff, 0xff);
	GemRB.SetLabelTextColor(StartWindow, BeginGameLabel, 0xff, 0xff, 0xff);
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,0,800,30, "REALMS2", "GemRB Ver 0.2.1", 1);
	GemRB.SetButtonSprites(StartWindow, ProtocolButton, "GBTNMED2", 0, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, NewGameButton, "GBTNMED2", 1, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, LoadGameButton, "GBTNMED2", 2, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, QuickLoadButton, "GBTNMED2", 3, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, JoinGameButton, "GBTNMED2", 0, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, OptionsButton, "GBTNMED2", 1, 1, 2, 0, 3);
	GemRB.SetButtonSprites(StartWindow, QuitGameButton, "GBTNMED2", 2, 1, 2, 0, 3);
	GemRB.SetControlStatus(StartWindow, ProtocolButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, NewGameButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, LoadGameButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, QuickLoadButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, QuitGameButton, IE_GUI_BUTTON_ENABLED);
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
	GemRB.SetText(StartWindow, GameModeLabel, 14451)
	GemRB.SetText(StartWindow, BeginGameLabel, 18296)
	GemRB.SetVisible(StartWindow, 1)
	GemRB.LoadMusicPL("Theme")
	GemRB.StartPL()
	return
