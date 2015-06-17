# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# NewLife.py - Character generation screen

###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
import CommonTables

CommonTables.Load()

NewLifeWindow = 0
QuitWindow = 0
TextArea = 0

StrLabel = 0
DexLabel = 0
ConLabel = 0
WisLabel = 0
IntLabel = 0
ChaLabel = 0
TotLabel = 0
AcLabel = 0
HpLabel = 0

StatTable = 0
Str = 0
Dex = 0
Con = 0
Wis = 0
Int = 0
Cha = 0
TotPoints = 0
AcPoints = 0
HpPoints = 0
strings = ("30","60","90","99","00")
extras = (30,60,90,99,100)

def OnLoad():
	global NewLifeWindow, QuitWindow, StatTable
	global TotPoints, AcPoints, HpPoints
	global Str, Dex, Con, Wis, Int, Cha
	global TotLabel, AcLabel, HpLabel
	global StrLabel, DexLabel, ConLabel, WisLabel, IntLabel, ChaLabel
	global TextArea

	GemRB.LoadGame(None)  #loading the base game
	StatTable = GemRB.LoadTable("abcomm")
	GemRB.LoadWindowPack("GUICG")
	#setting up confirmation window
	QuitWindow = GemRB.LoadWindow(1)
	QuitWindow.SetVisible(WINDOW_INVISIBLE)

	#setting up CG window
	NewLifeWindow = GemRB.LoadWindow(0)
	
	Str = 9
	Dex = 9
	Con = 9
	Wis = 9
	Int = 9
	Cha = 9
	TotPoints = 21
	
	StrLabel = NewLifeWindow.GetControl(0x10000018)
	DexLabel = NewLifeWindow.GetControl(0x1000001B)
	ConLabel = NewLifeWindow.GetControl(0x1000001C)
	WisLabel = NewLifeWindow.GetControl(0x1000001A)
	IntLabel = NewLifeWindow.GetControl(0x10000019)
	ChaLabel = NewLifeWindow.GetControl(0x1000001D)
	
	Button = NewLifeWindow.GetControl(2)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, StrPress)

	Button = NewLifeWindow.GetControl(3)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, IntPress)

	Button = NewLifeWindow.GetControl(4)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, WisPress)

	Button = NewLifeWindow.GetControl(5)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, DexPress)

	Button = NewLifeWindow.GetControl(6)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ConPress)

	Button = NewLifeWindow.GetControl(7)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ChaPress)

	Button = NewLifeWindow.GetControl(8)
	Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	Button.SetState(IE_GUI_BUTTON_LOCKED)
	Button.SetSprites("", 0, 0, 0, 0, 0)
	Button.SetText(5025)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, AcPress)

	Button = NewLifeWindow.GetControl(9)
	Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	Button.SetState(IE_GUI_BUTTON_LOCKED)
	Button.SetSprites("", 0, 0, 0, 0, 0)
	Button.SetText(5026)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, HpPress)

	Button = NewLifeWindow.GetControl(10)
	Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	Button.SetState(IE_GUI_BUTTON_LOCKED)
	Button.SetSprites("", 0, 0, 0, 0, 0)
	Button.SetText(5027)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, PointPress)

	Button = NewLifeWindow.GetControl(11) #str +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, StrPress)
	Button.SetVarAssoc("Pressed", 0)
	
	Button = NewLifeWindow.GetControl(13) #int +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, IntPress)
	Button.SetVarAssoc("Pressed", 1)
	
	Button = NewLifeWindow.GetControl(15) #wis +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, WisPress)
	Button.SetVarAssoc("Pressed", 2)
	
	Button = NewLifeWindow.GetControl(17) #dex +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, DexPress)
	Button.SetVarAssoc("Pressed", 3)
	
	Button = NewLifeWindow.GetControl(19) #con +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ConPress)
	Button.SetVarAssoc("Pressed", 4)
	
	Button = NewLifeWindow.GetControl(21) #chr +
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, IncreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ChaPress)
	Button.SetVarAssoc("Pressed", 5)
	
	Button = NewLifeWindow.GetControl(12) #str -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, StrPress)
	Button.SetVarAssoc("Pressed", 0)
	
	Button = NewLifeWindow.GetControl(14) #int -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, IntPress)
	Button.SetVarAssoc("Pressed", 1)
	
	Button = NewLifeWindow.GetControl(16) #wis -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, WisPress)
	Button.SetVarAssoc("Pressed", 2)
	
	Button = NewLifeWindow.GetControl(18) #dex -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, DexPress)
	Button.SetVarAssoc("Pressed", 3)
	
	Button = NewLifeWindow.GetControl( 20) #con -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ConPress)
	Button.SetVarAssoc("Pressed", 4)
	
	Button = NewLifeWindow.GetControl( 22) #chr -
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DecreasePress)
	Button.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, ChaPress)
	Button.SetVarAssoc("Pressed", 5)
	
	NewLifeLabel = NewLifeWindow.GetControl(0x10000023)
	NewLifeLabel.SetText(1899)
	
	TextArea = NewLifeWindow.GetControl(23)
	TextArea.SetText(18495)
	
	TotLabel = NewLifeWindow.GetControl(0x10000020)
	AcLabel = NewLifeWindow.GetControl(0x1000001E)
	HpLabel = NewLifeWindow.GetControl(0x1000001F)

	Label = NewLifeWindow.GetControl(0x10000021)
	Label.SetText(254)
	
	PhotoButton = NewLifeWindow.GetControl(35)
	PhotoButton.SetState(IE_GUI_BUTTON_LOCKED)
	PhotoButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	PhotoButton.SetEvent(IE_GUI_MOUSE_OVER_BUTTON, OverPhoto)
	PhotoButton.SetPicture("STPNOC")
	
	AcceptButton = NewLifeWindow.GetControl(0)
	AcceptButton.SetText(4192)
	AcceptButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, AcceptPress)
	AcceptButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	
	CancelButton = NewLifeWindow.GetControl(1)
	CancelButton.SetText(4196)	
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	
	UpdateLabels()
	
	NewLifeWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def UpdateLabels():
	global AcPoints, HpPoints

	if Str<=18:
		StrLabel.SetText(str(Str))
	else:
		StrLabel.SetText("18/"+strings[Str-19])
	DexLabel.SetText(str(Dex))
	ConLabel.SetText(str(Con))
	WisLabel.SetText(str(Wis))
	IntLabel.SetText(str(Int))
	ChaLabel.SetText(str(Cha))
	TotLabel.SetText(str(TotPoints))
	AcPoints = 10
	if Dex>14:
		AcPoints = AcPoints - (Dex-14)

	HpPoints = 20
	if Con>14:
		HpPoints = HpPoints + (Con-9)*2 + (Con-14)
	else:
		HpPoints = HpPoints + (Con-9)*2

	AcLabel.SetText(str(AcPoints))
	HpLabel.SetText(str(HpPoints))
	return
	

def OkButton():
	QuitWindow.SetVisible(WINDOW_INVISIBLE)
	NewLifeWindow.SetVisible(WINDOW_VISIBLE)
	return

def AcceptPress():
	if TotPoints:
		# Setting up the error window
		TextArea = QuitWindow.GetControl(0)
		TextArea.SetText(46782)

		Button = QuitWindow.GetControl(1)
		Button.SetText("")
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button = QuitWindow.GetControl(2)
		Button.SetText(46783)
		Button.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, OkButton)
		NewLifeWindow.SetVisible(WINDOW_GRAYED) #go dark
		QuitWindow.SetVisible(WINDOW_VISIBLE)
		return

	if NewLifeWindow:
		NewLifeWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	#set my character up
	MyChar = GemRB.CreatePlayer("charbase", 1 ) 

	if Str<=18:
		GemRB.SetPlayerStat(1, IE_STR, Str)
		GemRB.SetPlayerStat(1, IE_STREXTRA,0)
	else:
		GemRB.SetPlayerStat(1, IE_STR, 18)
		GemRB.SetPlayerStat(1, IE_STREXTRA,extras[Str-19])

	GemRB.SetPlayerStat(1, IE_INT, Int)
	GemRB.SetPlayerStat(1, IE_WIS, Wis)
	GemRB.SetPlayerStat(1, IE_DEX, Dex)
	GemRB.SetPlayerStat(1, IE_CON, Con)
	GemRB.SetPlayerStat(1, IE_CHR, Cha)

	#don't add con bonus, it will be calculated by the game
	#interestingly enough, the game adds only one level's con bonus
	if Con>14:
		x = 30
	else:
		x = 20+(Con-9)*2

	print "Setting max hp to: ",x
	GemRB.SetPlayerStat(1, IE_MAXHITPOINTS, x)
	#adding the remaining constitution bonus to the current hp
	#if Con>14:
	#	x = x+(Con-14)*3
	print "Setting current hp to: ",x
	GemRB.SetPlayerStat(1, IE_HITPOINTS, x)

	GemRB.FillPlayerInfo(1) #does all the rest
	#LETS PLAY!!
	GemRB.EnterGame()
	return

def CancelPress():
	# Setting up the confirmation window
	TextArea = QuitWindow.GetControl(0)
	TextArea.SetText(19406)

	Button = QuitWindow.GetControl(1)
	Button.SetText(23787)
	Button.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_SET)
	Button.SetState(IE_GUI_BUTTON_ENABLED)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, YesButton)

	Button = QuitWindow.GetControl(2)
	Button.SetText(23789)
	Button.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, OkButton)

	NewLifeWindow.SetVisible(WINDOW_GRAYED) #go dark
	QuitWindow.SetVisible(WINDOW_VISIBLE)
	return

def YesButton():
	if NewLifeWindow:
		NewLifeWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetNextScript("Start")
	return

def StrPress():
	TextArea.SetText(18489)
	s = Str
	if s>18:
		e=extras[s-19]
		s=18
	else:
		e=0

	x = CommonTables.StrMod.GetValue(s,0) + CommonTables.StrModEx.GetValue(e,0)
	y = CommonTables.StrMod.GetValue(s,1) + CommonTables.StrModEx.GetValue(e,1)
	if x==0:
		x=y
		y=0

	if e>60:
		s=19
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(s,0),x,y) )
	return

def IntPress():
	TextArea.SetText(18488)
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(Int,1),0,0) )
	return

def WisPress():
	TextArea.SetText(18490)
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(Wis,2),0,0) )
	return

def DexPress():
	Table = GemRB.LoadTable("dexmod")
	x = -Table.GetValue(Dex,2)
	TextArea.SetText(18487)
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(Dex,3),x,0) )
	return

def ConPress():
	Table = GemRB.LoadTable("hpconbon")
	x = Table.GetValue(Con-1,1)
	TextArea.SetText(18491)
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(Con,4),x,0) )
	return

def ChaPress():
	TextArea.SetText(1903)
	TextArea.Append("\n\n"+GemRB.StatComment(StatTable.GetValue(Cha,5),0,0) )
	return

def PointPress():
	TextArea.SetText(18492)
	return

def AcPress():
	TextArea.SetText(18493)
	return

def HpPress():
	TextArea.SetText(18494)
	return

def DecreasePress():
	global TotPoints
	global Sum, Str, Int, Wis, Dex, Con, Cha

	Pressed = GemRB.GetVar("Pressed")
	if Pressed == 0:
		Sum = Str
	if Pressed == 1:
		Sum = Int
	if Pressed == 2:
		Sum = Wis 
	if Pressed == 3:
		Sum = Dex
	if Pressed == 4:
		Sum = Con
	if Pressed == 5:
		Sum = Cha
	if Sum<=9:
		return
	TotPoints = TotPoints+1
	Sum = Sum-1
	if Pressed == 0:
		Str = Sum
		StrPress()
	if Pressed == 1:
		Int = Sum
		IntPress()
	if Pressed == 2:
		Wis = Sum
		WisPress()
	if Pressed == 3:
		Dex = Sum
		DexPress()
	if Pressed == 4:
		Con = Sum
		ConPress()
	if Pressed == 5:
		Cha = Sum
		ChaPress()
	UpdateLabels()
	return

def IncreasePress():
	global TotPoints
	global Sum, Str, Int, Wis, Dex, Con, Cha

	if TotPoints<=0:
		return
	Pressed = GemRB.GetVar("Pressed")
	if Pressed == 0:
		Sum = Str
		if Sum>=23:
			return
	if Pressed == 1:
		Sum = Int
		if Sum>=18:
			return
	if Pressed == 2:
		Sum = Wis 
		if Sum>=18:
			return
	if Pressed == 3:
		Sum = Dex
		if Sum>=18:
			return
	if Pressed == 4:
		Sum = Con
		if Sum>=18:
			return
	if Pressed == 5:
		Sum = Cha
		if Sum>=18:
			return
	TotPoints = TotPoints-1
	Sum = Sum+1
	if Pressed == 0:
		Str = Sum
		StrPress()
	if Pressed == 1:
		Int = Sum
		IntPress()
	if Pressed == 2:
		Wis = Sum
		WisPress()
	if Pressed == 3:
		Dex = Sum
		DexPress()
	if Pressed == 4:
		Con = Sum
		ConPress()
	if Pressed == 5:
		Cha = Sum
		ChaPress()
	UpdateLabels()
	return

def OverPhoto():
	global TextArea
	TextArea.SetText(18495)
