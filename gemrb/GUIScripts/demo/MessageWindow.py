import GemRB
import Tests
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
from GUIDefines import *

def OnLoad():
	MessageWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MessageWindow.SetFlags (WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	MessageWindow.AddAlias("MSGWIN")

	# set up some *initial* text (UpdateControlStatus will get called several times)
	TMessageTA = MessageWindow.GetControl(0)
	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	TMessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	TMessageTA.AddAlias("MsgSys", 0)
	TMessageTA.AddAlias("MTA", 0)
	
	results = Tests.RunTests ()
	TMessageTA.SetText ("[cap]D[/cap]emo " + "DEMO "*40 + "\n" + results)
	print results

def UpdateControlStatus():
	pass

