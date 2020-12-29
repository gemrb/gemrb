import GemRB
import Tests
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
import GUIINV
import GUIMA
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

MessageWindow = 0
#PortraitWindow = 0
#OptionsWindow = 0
TMessageTA = 0 # for dialog code

def OnLoad():
#	global PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)
	GemRB.LoadWindowPack(GUICommon.GetWindowPack())

#	GUICommonWindows.PortraitWindow = None
#	GUICommonWindows.ActionsWindow = None
#	GUICommonWindows.OptionsWindow = None
 
#	ActionsWindow = GemRB.LoadWindow(3)
#	OptionsWindow = GemRB.LoadWindow(0)
#	PortraitWindow = GUICommonWindows.OpenPortraitWindow(1)

#	GemRB.SetVar("PortraitWindow", PortraitWindow.ID)
#	GemRB.SetVar("ActionsWindow", ActionsWindow.ID)
#	GemRB.SetVar("OptionsWindow", OptionsWindow.ID)
	GemRB.SetVar("TopWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("FloatWindow", -1)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar("OptionsPosition", 0) #Left
	GemRB.SetVar("MessagePosition", 4) #BottomAdded
	GemRB.SetVar("OtherPosition", 5) #Inactivating
	GemRB.SetVar("TopPosition", 5) #Inactivating
	
#	GUICommonWindows.OpenActionsWindowControls (ActionsWindow)
#	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)

##	GUICommon.GameWindow.SetVisible (WINDOW_VISIBLE)

	UpdateControlStatus()

	# set up some *initial* text (UpdateControlStatus will get called several times)
	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	results = Tests.RunTests ()
	TMessageTA.SetText ("[cap]D[/cap]emo " + "DEMO "*40 + "\n" + results)
	print results

def UpdateControlStatus():
	global MessageWindow, TMessageTA

	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize()
	Override = GSFlags&GS_DIALOG

	GemRB.LoadWindowPack(GUICommon.GetWindowPack())

	TMessageWindow = GemRB.LoadWindow(0)
	TMessageTA = TMessageWindow.GetControl(0)

	hideflag = GemRB.HideGUI()
	MessageWindow = GemRB.GetVar("MessageWindow")
	MessageTA = GUIClasses.GTextArea(MessageWindow,GemRB.GetVar("MessageTextArea"))
	if MessageWindow > 0 and MessageWindow != TMessageWindow.ID:
		TMessageTA = MessageTA.SubstituteForControl(TMessageTA)
		GUIClasses.GWindow(MessageWindow).Unload()

	GemRB.SetVar("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar("MessageTextArea", TMessageTA.ID)
	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)

	PauseButton = TMessageWindow.GetControl (2)
	PauseButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: GemRB.GamePause (2, 0))
	PauseButton.SetAnimation ("loading")
	PauseButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)

	MapButton = TMessageWindow.GetControl (3)
	MapButton.SetText ("M")
	MapButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMA.OpenMapWindow)

	CenterButton = TMessageWindow.GetControl (4)
	CenterButton.SetText ("C")
	CenterButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR))

	InventoryButton = TMessageWindow.GetControl (5)
	InventoryButton.SetText ("I")
	InventoryButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.OpenInventoryWindow)

	if hideflag:
		GemRB.UnhideGUI()

