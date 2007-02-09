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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/NewLife.py,v 1.19 2007/02/09 19:37:34 avenger_teambg Exp $

# NewLife.py - Character generation screen

###################################################

import GemRB
from ie_stats import *

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

	GemRB.LoadGame(-1)  #loading the base game
	StatTable = GemRB.LoadTable("abcomm")
	GemRB.LoadWindowPack("GUICG")
	#setting up confirmation window
	QuitWindow = GemRB.LoadWindow(1)
	GemRB.SetVisible(QuitWindow, 0)

	#setting up CG window
	NewLifeWindow = GemRB.LoadWindow(0)
	
	Str = 9
	Dex = 9
	Con = 9
	Wis = 9
	Int = 9
	Cha = 9
	TotPoints = 21
	
	StrLabel = GemRB.GetControl(NewLifeWindow, 0x10000018)
	DexLabel = GemRB.GetControl(NewLifeWindow, 0x1000001B)
	ConLabel = GemRB.GetControl(NewLifeWindow, 0x1000001C)
	WisLabel = GemRB.GetControl(NewLifeWindow, 0x1000001A)
	IntLabel = GemRB.GetControl(NewLifeWindow, 0x10000019)
	ChaLabel = GemRB.GetControl(NewLifeWindow, 0x1000001D)
	
	Button = GemRB.GetControl(NewLifeWindow, 2)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "StrPress")

	Button = GemRB.GetControl(NewLifeWindow, 3)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "IntPress")

	Button = GemRB.GetControl(NewLifeWindow, 4)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "WisPress")

	Button = GemRB.GetControl(NewLifeWindow, 5)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "DexPress")

	Button = GemRB.GetControl(NewLifeWindow, 6)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ConPress")

	Button = GemRB.GetControl(NewLifeWindow, 7)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ChaPress")

	Button = GemRB.GetControl(NewLifeWindow, 8)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5025)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_MOUSE_OVER_BUTTON, "AcPress")

	Button = GemRB.GetControl(NewLifeWindow, 9)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5026)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_MOUSE_OVER_BUTTON, "HpPress")

	Button = GemRB.GetControl(NewLifeWindow, 10)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5027)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_MOUSE_OVER_BUTTON, "PointPress")

	Button = GemRB.GetControl(NewLifeWindow, 11) #str +
	GemRB.SetEvent(NewLifeWindow, Button, IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "StrPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 0)
	
	Button = GemRB.GetControl(NewLifeWindow, 13) #int +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "IntPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 1)
	
	Button = GemRB.GetControl(NewLifeWindow, 15) #wis +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "WisPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 2)
	
	Button = GemRB.GetControl(NewLifeWindow, 17) #dex +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "DexPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 3)
	
	Button = GemRB.GetControl(NewLifeWindow, 19) #con +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ConPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 4)
	
	Button = GemRB.GetControl(NewLifeWindow, 21) #chr +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ChaPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 5)
	
	Button = GemRB.GetControl(NewLifeWindow, 12) #str -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "StrPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 0)
	
	Button = GemRB.GetControl(NewLifeWindow, 14) #int -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "IntPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 1)
	
	Button = GemRB.GetControl(NewLifeWindow, 16) #wis -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "WisPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 2)
	
	Button = GemRB.GetControl(NewLifeWindow, 18) #dex -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "DexPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 3)
	
	Button = GemRB.GetControl(NewLifeWindow,  20) #con -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ConPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 4)
	
	Button = GemRB.GetControl(NewLifeWindow,  22) #chr -
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_MOUSE_OVER_BUTTON, "ChaPress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 5)
	
	NewLifeLabel = GemRB.GetControl(NewLifeWindow, 0x10000023)
	GemRB.SetText(NewLifeWindow, NewLifeLabel, 1899)
	
	TextArea = GemRB.GetControl(NewLifeWindow, 23)
	GemRB.SetText(NewLifeWindow, TextArea, 18495)
	
	TotLabel = GemRB.GetControl(NewLifeWindow, 0x10000020)
	AcLabel = GemRB.GetControl(NewLifeWindow, 0x1000001E)
	HpLabel = GemRB.GetControl(NewLifeWindow, 0x1000001F)

	Label = GemRB.GetControl(NewLifeWindow, 0x10000021)
	GemRB.SetText(NewLifeWindow, Label, 254)
	
	PhotoButton = GemRB.GetControl(NewLifeWindow, 35)
	GemRB.SetButtonState(NewLifeWindow, PhotoButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(NewLifeWindow, PhotoButton, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture(NewLifeWindow, PhotoButton, "STPNOC")
	
	AcceptButton = GemRB.GetControl(NewLifeWindow, 0)
	GemRB.SetText(NewLifeWindow, AcceptButton, 4192)
	GemRB.SetEvent(NewLifeWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "AcceptPress")
	GemRB.SetButtonFlags(NewLifeWindow, AcceptButton, IE_GUI_BUTTON_DEFAULT,OP_OR)
	
	CancelButton = GemRB.GetControl(NewLifeWindow, 1)
	GemRB.SetText(NewLifeWindow, CancelButton, 4196)	
	GemRB.SetEvent(NewLifeWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	
	UpdateLabels()
	
	GemRB.SetVisible(NewLifeWindow, 1)
	return
	
def UpdateLabels():
	global AcPoints, HpPoints

	if Str<=18:
		GemRB.SetText(NewLifeWindow, StrLabel, str(Str))
	else:
		GemRB.SetText(NewLifeWindow, StrLabel,"18/"+strings[Str-19])
	GemRB.SetText(NewLifeWindow, DexLabel, str(Dex))
	GemRB.SetText(NewLifeWindow, ConLabel, str(Con))
	GemRB.SetText(NewLifeWindow, WisLabel, str(Wis))
	GemRB.SetText(NewLifeWindow, IntLabel, str(Int))
	GemRB.SetText(NewLifeWindow, ChaLabel, str(Cha))
	GemRB.SetText(NewLifeWindow, TotLabel, str(TotPoints))
	AcPoints = 10
	if Dex>14:
		AcPoints = AcPoints - (Dex-14)

	HpPoints = 20
	if Con>14:
		HpPoints = HpPoints + (Con-9)*2 + (Con-14)
	else:
		HpPoints = HpPoints + (Con-9)*2

	GemRB.SetText(NewLifeWindow, AcLabel, str(AcPoints))
	GemRB.SetText(NewLifeWindow, HpLabel, str(HpPoints))
	return
	

def OkButton():
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(NewLifeWindow, 1)
	return

def AcceptPress():
	if TotPoints:
		# Setting up the error window
		TextArea = GemRB.GetControl(QuitWindow, 0)
		GemRB.SetText(QuitWindow, TextArea, 46782)

		Button = GemRB.GetControl(QuitWindow,1)
		GemRB.SetText(QuitWindow, Button, "")
		GemRB.SetButtonFlags(QuitWindow, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)
		GemRB.SetButtonState(QuitWindow, Button, IE_GUI_BUTTON_DISABLED)
		Button = GemRB.GetControl(QuitWindow, 2)
		GemRB.SetText(QuitWindow, Button, 46783)
		GemRB.SetButtonFlags(QuitWindow, Button, IE_GUI_BUTTON_DEFAULT,OP_OR)
		GemRB.SetEvent(QuitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OkButton")
		GemRB.SetVisible(NewLifeWindow,2) #go dark
		GemRB.SetVisible(QuitWindow,1)
		return

	GemRB.UnloadWindow(NewLifeWindow)
	GemRB.UnloadWindow(QuitWindow)
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
	TextArea = GemRB.GetControl(QuitWindow, 0)
	GemRB.SetText(QuitWindow, TextArea, 19406)

	Button = GemRB.GetControl(QuitWindow,1)
	GemRB.SetText(QuitWindow, Button, 23787)
	GemRB.SetButtonFlags(QuitWindow, Button, IE_GUI_BUTTON_DEFAULT,OP_SET)
	GemRB.SetButtonState(QuitWindow, Button, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(QuitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "YesButton")

	Button = GemRB.GetControl(QuitWindow, 2)
	GemRB.SetText(QuitWindow, Button, 23789)
	GemRB.SetButtonFlags(QuitWindow, Button, IE_GUI_BUTTON_DEFAULT,OP_OR)
	GemRB.SetEvent(QuitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OkButton")

	GemRB.SetVisible(NewLifeWindow,2) #go dark
	GemRB.SetVisible(QuitWindow,1)
	return

def YesButton():
	GemRB.UnloadWindow(NewLifeWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetNextScript("Start")
	return

def StrPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18489)
	Table = GemRB.LoadTable("strmod")
	TableEx = GemRB.LoadTable("strmodex")
	s = Str
	if s>18:
		e=extras[s-19]
		s=18
	else:
		e=0

	x = GemRB.GetTableValue(Table,s,0) + GemRB.GetTableValue(TableEx, e,0)
	y = GemRB.GetTableValue(Table,s,1) + GemRB.GetTableValue(TableEx, e,1)
	if x==0:
		x=y
		y=0

	if e>60:
		s=19
	GemRB.TextAreaAppend(NewLifeWindow, TextArea,"\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,s,0),x,y) )
	return

def IntPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18488)
	GemRB.TextAreaAppend(NewLifeWindow, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Int,1),0,0) )
	return

def WisPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18490)
	GemRB.TextAreaAppend(NewLifeWindow, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Wis,2),0,0) )
	return

def DexPress():
	Table = GemRB.LoadTable("dexmod")
	x = -GemRB.GetTableValue(Table,Dex,2)
	GemRB.SetText(NewLifeWindow, TextArea, 18487)
	GemRB.TextAreaAppend(NewLifeWindow, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Dex,3),x,0) )
	return

def ConPress():
	Table = GemRB.LoadTable("hpconbon")
	x = GemRB.GetTableValue(Table,Con-1,1)
	GemRB.SetText(NewLifeWindow, TextArea, 18491)
	GemRB.TextAreaAppend(NewLifeWindow, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Con,4),x,0) )
	return

def ChaPress():
	GemRB.SetText(NewLifeWindow, TextArea, 1903)
	GemRB.TextAreaAppend(NewLifeWindow, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Cha,5),0,0) )
	return

def PointPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18492)
	return

def AcPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18493)
	return

def HpPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18494)
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

