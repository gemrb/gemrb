import GemRB

from GUICommonWindows import *
from GUIJRNL import *
from GUIMA import *
from GUIMG import *
from GUIINV import *
from GUIOPT import *
from GUIPR import *
from GUIREC import *
from GUISTORE import *
from GUIWORLD import *

MessageWindow = 0
MessageTA = 0

def OnLoad():
	global MessageWindow, MessageTA
	GemRB.LoadWindowPack("GUIW08")
	OptionsWindow = MessageWindow = GemRB.LoadWindow(0)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)

	ActionsWindow = PortraitWindow = GemRB.LoadWindow(1)
	
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("MessageTextArea", MessageTA)
	
	PopulatePortraitWindow(PortraitWindow)
	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)


	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("PortraitPosition", 4)
	
	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return
