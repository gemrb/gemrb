import GemRB

from GUICommonWindows import *
import GUICommonWindows
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

	OpenPortraitWindow()
	ActionsWindow = PortraitWindow = GUICommonWindows.PortraitWindow

	#ActionsWindow = PortraitWindow = GemRB.LoadWindow(1)
	
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("TopWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("FloatWindow", -1)
	GemRB.SetVar("ActionsPosition", -1) #already handled in portraitwindow
	GemRB.SetVar("OptionsPosition", -1) #already handled in messagewindow
	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("OtherPosition", 5) #Inactivating
	GemRB.SetVar("TopPosition", 5) #Inactivating

	GemRB.SetVar("MessageTextArea", MessageTA)
	
	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)

	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return
