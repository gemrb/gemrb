import GemRB
import Tests
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
from GUIDefines import *

TMessageTA = None

def OnLoad():
	UpdateControlStatus()

	# set up some *initial* text (UpdateControlStatus will get called several times)
	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	results = Tests.RunTests ()
	TMessageTA.SetText ("[cap]D[/cap]emo " + "DEMO "*40 + "\n" + results)
	print results

def UpdateControlStatus():
	global TMessageTA

	GSFlags = GemRB.GetGUIFlags()
	GSFlags = GSFlags - GS_LARGEDIALOG
	Override = GSFlags&GS_DIALOG

	MessageWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MessageWindow.AddAlias("MSGWIN")

	TMessageTA = MessageWindow.GetControl(0)
	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	TMessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	TMessageTA.AddAlias("MsgSys", 0)
	TMessageTA.AddAlias("MTA", 0)
