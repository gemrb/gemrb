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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIREC.py,v 1.4 2004/04/11 12:42:23 edheldil Exp $


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack

# GUIREC:
# 0,1,2 - sommon windows (time, message, menu)
# 3 - main statistics window
# 4 - level up win
# 5 - info - kills, weapons ...
# 6 - dual class list ???
# 7 - class panel
# 8 - skills panel
# 9 - choose mage spells panel
# 10 - some small win, 1 button
# 11 - some small win, 2 buttons
# 12 - biography?
# 13 - specialist mage panel
# 14 - proficiencies
# 15 - some 2 panel window
# 16 - some 2 panel window
# 17 - some 2 panel window


# MainWindow:
# 0 - main textarea
# 1 - its scrollbar
# 2 - WMCHRP character portrait
# 5 - STALIGN alignment
# 6 - STFCTION faction
# 7,8,9 - STCSTM (info, reform party, level up)
# 0x1000000a - name
#0x1000000b - ac
#0x1000000c, 0x1000000d hp now, hp max
#0x1000000e str
#0x1000000f int
#0x10000010 wis
#0x10000011 dex
#0x10000012 con
#0x10000013 chr
#x10000014 race
#x10000015 sex
#0x10000016 class

#31-36 stat buts
#37 ac but
#38 hp but?


###################################################
import GemRB
from GUIDefines import *
import ie_stats
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler, RunSelectionChangeHandler
from GUIWORLD import OpenReformPartyWindow

#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
#import GUICommonWindows

###################################################
RecordsWindow = None
InformationWindow = None
BiographyWindow = None

###################################################
def OpenRecordsWindow ():
	global RecordsWindow

	GemRB.HideGUI ()
	
	if RecordsWindow != None:
		if InformationWindow: OpenInformationWindow ()
		
		GemRB.UnloadWindow (RecordsWindow)
		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		SetSelectionChangeHandler (None)
		
		GemRB.UnhideGUI()
		return	

	GemRB.LoadWindowPack ("GUIREC")
	RecordsWindow = Window = GemRB.LoadWindow (3)
        GemRB.SetVar ("OtherWindow", RecordsWindow)


	# Information
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 4245)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")
	
	# Reform Party
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 4244)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	# Level Up
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 4246)

	# stat buttons
	for i in range (6):
		Button = GemRB.GetControl (Window, 31 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# AC button
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# HP button
	Button = GemRB.GetControl (Window, 38)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()

	#GemRB.SetVisible (Window, 1)
	GemRB.UnhideGUI ()


def UpdateRecordsWindow ():
	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GetActorByPartyID (GemRB.GameGetSelectedPCSingle ())
	
	# help, info textarea
	Text = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Text, "Pokus ....")

	# name
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))

	# portrait
	Image = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags(Window, Image, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture (Window, Image, GetActorPortrait (pc, 1))

	# armorclass
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, ie_stats.IE_ARMORCLASS)))

	# hp now
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, ie_stats.IE_HITPOINTS)))

	# hp max
	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, ie_stats.IE_MAXHITPOINTS)))

	# stats

	sstr = GemRB.GetPlayerStat (pc, ie_stats.IE_STR)
	sstrx = GemRB.GetPlayerStat (pc, ie_stats.IE_STREXTRA)
	if (sstrx > 0):
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	sint = GemRB.GetPlayerStat (pc, ie_stats.IE_INT)
	swis = GemRB.GetPlayerStat (pc, ie_stats.IE_WIS)
	sdex = GemRB.GetPlayerStat (pc, ie_stats.IE_DEX)
	scon = GemRB.GetPlayerStat (pc, ie_stats.IE_CON)
	schr = GemRB.GetPlayerStat (pc, ie_stats.IE_CHR)

	stats = (sstr, sint, swis, sdex, scon, schr)
	for i in range (6):
		Label = GemRB.GetControl (Window, 0x1000000e + i)
		GemRB.SetText (Window, Label, str (stats[i]))

	# race
	RaceTable = GemRB.LoadTable ("RACES")
	text = GemRB.GetTableValue (RaceTable, GemRB.GetPlayerStat (pc, ie_stats.IE_RACE) - 1, 0)
	GemRB.UnloadTable (RaceTable)
	
	Label = GemRB.GetControl (Window, 0x10000014)
	GemRB.SetText (Window, Label, text)


	# sex
	GenderTable = GemRB.LoadTable ("GENDERS")
	text = GemRB.GetTableValue (GenderTable, GemRB.GetPlayerStat (pc, ie_stats.IE_SEX) - 1, 0)
	GemRB.UnloadTable (GenderTable)
	
	Label = GemRB.GetControl (Window, 0x10000015)
	GemRB.SetText (Window, Label, text)
	print "SEX:", GemRB.GetPlayerStat (pc, ie_stats.IE_SEX)


	# class
	ClassTable = GemRB.LoadTable ("CLASS")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, ie_stats.IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x10000016)
	GemRB.SetText (Window, Label, text)
	print "CLASS:", GemRB.GetPlayerStat (pc, ie_stats.IE_CLASS)

	# alignment
	align = GemRB.GetPlayerStat (pc, ie_stats.IE_ALIGNMENT)
	print 'ALIGN:', align
	ss = GemRB.LoadSymbol ("ALIGN")
	sym = GemRB.GetSymbolValue (ss, align)
	print "ALIGN SYM:", sym

	align_anim = (3 * int (align / 16) + align % 16) - 4
	print 'ALIGN  ANIM:', align_anim
	
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites (Window, Button, 'STALIGN', 0, align_anim, 0, 0, 0)
	
	# faction
	faction = GemRB.GetPlayerStat (pc, ie_stats.IE_FACTION)
	print 'FACTION:', faction

	FactionTable = GemRB.LoadTable ("FACTIONS")
	frame = GemRB.GetTableValue (FactionTable, faction, 1)
	GemRB.UnloadTable (FactionTable)
	
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites (Window, Button, 'STFCTION', 0, frame, 0, 0, 0)

def OnRecordsStr ():
	# 20106 faction

	# 34584 no align 1/8
	# 34588 Dustmen 4/8 
	# 34585 godsmen 8/8
	# 34587 Sensates 3/8
	# 34589 Anarchists
	# 34590 Xaositects
	# 3789 indeps 7/8
	# 34586 Mercykillers


	# 33657 LG desc
	
	pass

def stattext():
	# 19672 Level <LEVEL> Spells

	
	# 48156 Level
	# 19673 Experience
	# 19674 Next Level
	
	# 59856 Current State
	
	# 67049 AC Bonuses
	#   67204 AC vs. Slashing
	#   67205 AC vs. Piercing
	#   67206 AC vs. Crushing
	#   67207 AC vs. Missile
	
	# 67208 Resistances
	#   67209 Normal Fire
	#   67210 Magic Fire
	#   67211 Normal Cold
	#   67212 Magic Cold
	#   67213 Electricity
	#   67214 Acid
	#   67215 Magic
	#   67216 Slashing Attacks
	#   67217 Piercing Attacks
	#   67218 Crushing Attacks
	#   67219 Missile Attacks

	# 4220 Proficiencies
	#   4208 THAC0
	#   4209 Number of Attacks
	#   4210 Lore
	#   4211 Open Locks
	#   4212 Stealth
	#   4213 Find/Remove Traps
	#   4214 Pick Pockets
	#   4215 Tracking
	#   4216 Reputation
	#   4217 Turn Undead Level
	#   4218 Lay on Hands Amount
	#   4219 Backstab Damage

	# 4221 Saving Throws
	#   4222 Paralyze/Poison/Death
	#   4223 Rod/Staff/Wand
	#   4224 Petrify/Polymorph
	#   4225 Breath Weapon
	#   4226 Spells
	
	# 4227 Weapon Proficiencies
	#   55011 Unused Slots
	#   33642 Fist
	#   33649 Edged Weapon
	#   33651 Hammer
	#   44990 Axe
	#   33653 Club
	#   33655 Bow

	# 4228 Ability Bonuses
	#   4229 To Hit
	#   4230 Damage
	#   4231 Open Doors
	#   4232 Weight Allowance
	#   4233 Armor Class Bonus
	#   4234 Missile Adjustment
	#   4236 CON HP Bonus/Level
	#   4240 Reaction

	# 4238 Magical Defense Adjustment
	#   4239 Bonus Priest Spells

	# 4237 Chance to learn spell

	# ??? 4235 Reaction Adjustment

	pass


def OpenInformationWindow ():
	global InformationWindow
	
	GemRB.HideGUI ()
	
	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		GemRB.UnloadWindow (InformationWindow)
		InformationWindow = None
		GemRB.SetVar ("OtherWindow", RecordsWindow)
		
		GemRB.UnhideGUI()
		return

	InformationWindow = Window = GemRB.LoadWindow (5)
        GemRB.SetVar ("OtherWindow", InformationWindow)

	GemRB.UnhideGUI ()

	# Biography
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4247)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")


	#GemRB.SetVisible (Window, 1)
	

def OpenBiographyWindow ():
	global BiographyWindow

	GemRB.HideGUI ()
	
	if BiographyWindow != None:
		GemRB.UnloadWindow (BiographyWindow)
		BiographyWindow = None
		GemRB.SetVar ("OtherWindow", InformationWindow)
		
		GemRB.UnhideGUI()
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)
        GemRB.SetVar ("OtherWindow", BiographyWindow)

	GemRB.UnhideGUI ()
	

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	#GemRB.SetVisible (Window, 1)
	

###################################################
# End of file GUIREC.py
