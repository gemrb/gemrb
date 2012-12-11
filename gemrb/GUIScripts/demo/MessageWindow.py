import GemRB
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
from GUIDefines import *

MessageWindow = 0
#PortraitWindow = 0
#OptionsWindow = 0
TMessageTA = 0 # for dialog code

def OnLoad():
#	global PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
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

def ScrollUp ():
	TMessageWindow = GemRB.GetVar("MessageWindow")
	TMessageTA = GUIClasses.GTextArea(TMessageWindow,GemRB.GetVar("MessageTextArea"))
	TMessageTA.Scroll(-1)

def ScrollDown ():
	TMessageWindow = GemRB.GetVar("MessageWindow")
	TMessageTA = GUIClasses.GTextArea(TMessageWindow,GemRB.GetVar("MessageTextArea"))
	TMessageTA.Scroll(1)

def UpdateControlStatus():
	global MessageWindow, TMessageTA

	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize()
	Expand = GS_LARGEDIALOG
	GSFlags = GSFlags-Expand
	Override = GSFlags&GS_DIALOG

	MessageWindow = GemRB.GetVar("MessageWindow")

	GemRB.LoadWindowPack(GUICommon.GetWindowPack())

	if Expand == GS_LARGEDIALOG:
		TMessageWindow = GemRB.LoadWindow(0)
		TMessageTA = TMessageWindow.GetControl(0)

	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL)
	TMessageTA.SetHistory(100)

	hideflag = GemRB.HideGUI()
	MessageTA = GUIClasses.GTextArea(MessageWindow,GemRB.GetVar("MessageTextArea"))
	if MessageWindow>0 and MessageWindow!=TMessageWindow.ID:
		MessageTA.MoveText(TMessageTA)
		GUIClasses.GWindow(MessageWindow).Unload()

	GemRB.SetVar("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar("MessageTextArea", TMessageTA.ID)
	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)

	if hideflag:
		GemRB.UnhideGUI()

