import GemRB
import Tests
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
import GUIINV
import GUIMA
from GUIDefines import *

def OnLoad():
	MessageWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MessageWindow.SetFlags (WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	MessageWindow.AddAlias("MSGWIN")
	MessageWindow.AddAlias("HIDE_CUT", 0)

	# set up some *initial* text (UpdateControlStatus will get called several times)
	TMessageTA = MessageWindow.GetControl(0)
	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	TMessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	TMessageTA.AddAlias("MsgSys", 0)
	TMessageTA.AddAlias("MTA", 0)
	TMessageTA.SetColor(ColorRed, TA_COLOR_OPTIONS)
	TMessageTA.SetColor(ColorWhite, TA_COLOR_HOVER)
	
	results = Tests.RunTests ()
	TMessageTA.SetText ("[cap]D[/cap]emo " + "DEMO "*40 + "\n" + results)

	PauseButton = MessageWindow.GetControl (2)
	PauseButton.OnPress (lambda: GemRB.GamePause (2, 0))
	PauseButton.SetAnimation ("loading")
	PauseButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NORMAL, OP_SET)

	MapButton = MessageWindow.GetControl (3)
	MapButton.SetText ("M")
	MapButton.OnPress (GUIMA.OpenMapWindow)
	MapButton.OnRightPress (lambda: GemRB.MoveToArea ("ar0110"))

	CenterButton = MessageWindow.GetControl (4)
	CenterButton.SetText ("C")
	CenterButton.OnPress (lambda: GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR))

	InventoryButton = MessageWindow.GetControl (5)
	InventoryButton.SetText ("I")
	InventoryButton.OnPress (GUIINV.OpenInventoryWindow)

def UpdateControlStatus():
	pass
