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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIREC.py,v 1.8 2004/04/27 06:23:10 edheldil Exp $


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
import string
import GemRB
from GUIDefines import *
from ie_stats import *
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


stats_overview = None
faction_help = ''
alignment_help = ''

def UpdateRecordsWindow ():
	global stats_overview, faction_help, alignment_help
	
	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	pc = pc + 1; 
	print "PC:", pc
	
	# name
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Image = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags(Window, Image, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture (Window, Image, GetActorPortrait (pc, 1))

	# armorclass
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))

	# hp now
	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))

	# hp max
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))

	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	if (sstrx > 0):
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)

	stats = (sstr, sint, swis, sdex, scon, schr)
	for i in range (6):
		Label = GemRB.GetControl (Window, 0x1000000e + i)
		GemRB.SetText (Window, Label, str (stats[i]))

	# race
	RaceTable = GemRB.LoadTable ("RACES")
	text = GemRB.GetTableValue (RaceTable, GemRB.GetPlayerStat (pc, IE_RACE) - 1, 0)
	GemRB.UnloadTable (RaceTable)
	
	Label = GemRB.GetControl (Window, 0x10000014)
	GemRB.SetText (Window, Label, text)


	# sex
	GenderTable = GemRB.LoadTable ("GENDERS")
	text = GemRB.GetTableValue (GenderTable, GemRB.GetPlayerStat (pc, IE_SEX) - 1, 0)
	GemRB.UnloadTable (GenderTable)
	
	Label = GemRB.GetControl (Window, 0x10000015)
	GemRB.SetText (Window, Label, text)
	print "SEX:", GemRB.GetPlayerStat (pc, IE_SEX)


	# class
	ClassTable = GemRB.LoadTable ("CLASS")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x10000016)
	GemRB.SetText (Window, Label, text)
	print "CLASS:", GemRB.GetPlayerStat (pc, IE_CLASS)

	# alignment
	align = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	print 'ALIGN:', align
	ss = GemRB.LoadSymbol ("ALIGN")
	sym = GemRB.GetSymbolValue (ss, align)
	print "ALIGN SYM:", sym

	AlignmentTable = GemRB.LoadTable ("ALIGNS")
	#print "ALIGN DESC:", GemRB.GetTableValue (AlignmentTable, align + 1, 0)
	#print "ALIGN DESC2:", GemRB.GetTableValue (AlignmentTable, sym, 'DESC_REF')
	alignment_help = GemRB.GetString (GemRB.GetTableValue (AlignmentTable, sym, 'DESC_REF'))
	frame = (3 * int (align / 16) + align % 16) - 4
	
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites (Window, Button, 'STALIGN', 0, frame, 0, 0, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, "OnRecordsHelpAlignment")
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")


	# faction
	faction = GemRB.GetPlayerStat (pc, IE_FACTION)
	print 'FACTION:', faction

	FactionTable = GemRB.LoadTable ("FACTIONS")
	faction_help = GemRB.GetString (GemRB.GetTableValue (FactionTable, faction, 0))
	frame = GemRB.GetTableValue (FactionTable, faction, 1)
	GemRB.UnloadTable (FactionTable)
	
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites (Window, Button, 'STFCTION', 0, frame, 0, 0, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, "OnRecordsHelpFaction")
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")
	
	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Text, stats_overview)

# puts default info to textarea (overview of PC's bonuses, saves, etc.
def OnRecordsButtonLeave ():
	Window = RecordsWindow
	# help, info textarea
	Text = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Text, stats_overview)
	

def OnRecordsHelpFaction ():
	Window = RecordsWindow
	#Help = GemRB.GetVar ("ControlHelp")
	Help = GemRB.GetString (20106) + "\n\n" + faction_help
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	
	# 20106 faction

	# 34584 no align 1/8
	# 34588 Dustmen 4/8 
	# 34585 godsmen 8/8
	# 34587 Sensates 3/8
	# 34589 Anarchists 5/8
	# 34590 Xaositects 6/8
	# 3789 indeps 7/8
	# 34586 Mercykillers 2/8


def OnRecordsHelpAlignment ():
	Window = RecordsWindow
	#Help = GemRB.GetVar ("ControlHelp")
	Help = GemRB.GetString (20105) + "\n\n" + alignment_help
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	
	# 20105 alignment

	# 33657 LG desc
	# 33927 NG
	# 33928 CG
	# 33929 LN
	# 33948 TN
	# 33949 CN
	# 33950 LE
	# 33951 NE
	# 33952 CE

	# 33930 - attr comments


def GetStatOverview (pc):
	won = "[color=FFFFFF]"
	woff = "[/color]"
	str_None = GemRB.GetString (41275)
	
	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)

	stats = []
	# 19672 Level <LEVEL> Spells

	ClassTable = GemRB.LoadTable ("CLASS")
	ClassName = GemRB.GetString (GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0))
	GemRB.UnloadTable (ClassTable)

	# 48156 Level
	Level = GemRB.GetString (48156) + ': ' + str (GemRB.GetPlayerStat (pc, IE_LEVEL))

	# 19673 Experience
	Experience = GemRB.GetString (19673) + ': ' + str (GemRB.GetPlayerStat (pc, IE_XP))
	# 19674 Next Level

	Main = ClassName + "\n" + Level + "\n" + Experience + "\n\n"
	
	# 59856 Current State
	CurrentState = won + GemRB.GetString (59856) + woff + "\n\n"


	# 67049 AC Bonuses
	stats.append (67049)
	#   67204 AC vs. Slashing
	stats.append ((67204, GS (IE_ACSLASHINGMOD), ''))
	#   67205 AC vs. Piercing
	stats.append ((67205, GS (IE_ACPIERCINGMOD), ''))
	#   67206 AC vs. Crushing
	stats.append ((67206, GS (IE_ACCRUSHINGMOD), ''))
	#   67207 AC vs. Missile
	stats.append ((67207, GS (IE_ACMISSILEMOD), ''))
	stats.append (None)

	
	# 67208 Resistances
	stats.append (67208)
	#   67209 Normal Fire
	stats.append ((67209, GS (IE_RESISTFIRE), '%'))
	#   67210 Magic Fire
	stats.append ((67210, GS (IE_RESISTMAGICFIRE), '%'))
	#   67211 Normal Cold
	stats.append ((67211, GS (IE_RESISTCOLD), '%'))
	#   67212 Magic Cold
	stats.append ((67212, GS (IE_RESISTMAGICCOLD), '%'))
	#   67213 Electricity
	stats.append ((67213, GS (IE_RESISTELECTRICITY), '%'))
	#   67214 Acid
	stats.append ((67214, GS (IE_RESISTACID), '%'))
	#   67215 Magic
	stats.append ((67215, GS (IE_RESISTMAGIC), '%'))
	#   67216 Slashing Attacks
	stats.append ((67216, GS (IE_RESISTSLASHING), '%'))
	#   67217 Piercing Attacks
	stats.append ((67217, GS (IE_RESISTPIERCING), '%'))
	#   67218 Crushing Attacks
	stats.append ((67218, GS (IE_RESISTCRUSHING), '%'))
	#   67219 Missile Attacks
	stats.append ((67219, GS (IE_RESISTMISSILE), '%'))
	stats.append (None)

	# 4220 Proficiencies
	stats.append (4220)
	#   4208 THAC0
	stats.append ((4208, GS (IE_THAC0), ''))
	#   4209 Number of Attacks
	stats.append ((4209, GS (IE_NUMBEROFATTACKS), ''))
	#   4210 Lore
	stats.append ((4210, GS (IE_LORE), ''))
	#   4211 Open Locks
	stats.append ((4211, GS (IE_LOCKPICKING), ''))
	#   4212 Stealth
	stats.append ((4212, GS (IE_STEALTH), ''))
	#   4213 Find/Remove Traps
	stats.append ((4213, GS (IE_TRAPS), ''))
	#   4214 Pick Pockets
	stats.append ((4214, GS (IE_PICKPOCKET), ''))
	#   4215 Tracking
	stats.append ((4215, GS (IE_TRACKING), ''))
	#   4216 Reputation
	stats.append ((4216, GS (IE_REPUTATION), ''))
	#   4217 Turn Undead Level
	stats.append ((4217, GS (IE_TURNUNDEADLEVEL), ''))
	#   4218 Lay on Hands Amount
	stats.append ((4218, GS (IE_LAYONHANDSAMOUNT), ''))
	#   4219 Backstab Damage
	stats.append ((4219, GS (IE_BACKSTABDAMAGEMULTIPLIER), ''))
	stats.append (None)

	# 4221 Saving Throws
	stats.append (4221)
	#   4222 Paralyze/Poison/Death
	stats.append ((4222, GS (IE_SAVEVSDEATH), ''))
	#   4223 Rod/Staff/Wand
	stats.append ((4223, GS (IE_SAVEVSWANDS), ''))
	#   4224 Petrify/Polymorph
	stats.append ((4224, GS (IE_SAVEVSPOLY), ''))
	#   4225 Breath Weapon
	stats.append ((4225, GS (IE_SAVEVSBREATH), ''))
	#   4226 Spells
	stats.append ((4226, GS (IE_SAVEVSSPELL), ''))
	stats.append (None)
	
	# 4227 Weapon Proficiencies
	stats.append (4227)
	#   55011 Unused Slots
	#   33642 Fist
	#   33649 Edged Weapon
	#   33651 Hammer
	#   44990 Axe
	stats.append ((44990, GS (IE_PROFICIENCYAXE), ''))
	#   33653 Club
	#   33655 Bow
	stats.append (None)
	
	# 4228 Ability Bonuses
	stats.append (4228)
	#   4229 To Hit
	#   4230 Damage
	#   4231 Open Doors
	#   4232 Weight Allowance
	#   4233 Armor Class Bonus
	#   4234 Missile Adjustment
	stats.append ((4234, GS (IE_ACMISSILEMOD), ''))
	#   4236 CON HP Bonus/Level
	#   4240 Reaction
	stats.append (None)

	# 4238 Magical Defense Adjustment
	stats.append (4238)
	#   4239 Bonus Priest Spells
	stats.append ((4239, GS (IE_CASTINGLEVELBONUSCLERIC), ''))
	stats.append (None)
	
	# 4237 Chance to learn spell
	#SpellLearnChance = won + GemRB.GetString (4237) + woff

	# ??? 4235 Reaction Adjustment

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			res.append (GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append (won + GemRB.GetString (s) + woff)
				lines = 0
			else:
				if not lines:
					res.append (str_None)
				res.append ("")
				lines = 0

	return Main + CurrentState + string.join (res, "\n")
	pass


def OpenInformationWindow ():
	global InformationWindow
	
	GemRB.HideGUI ()
	
	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		GemRB.UnloadWindow (InformationWindow)
		InformationWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI()
		return

	InformationWindow = Window = GemRB.LoadWindow (5)
        GemRB.SetVar ("FloatWindow", InformationWindow)


	# Biography
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4247)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")


	#GemRB.SetVisible (Window, 1)
	GemRB.UnhideGUI ()
	

def OpenBiographyWindow ():
	global BiographyWindow

	GemRB.HideGUI ()
	
	if BiographyWindow != None:
		GemRB.UnloadWindow (BiographyWindow)
		BiographyWindow = None
		GemRB.SetVar ("FloatWindow", InformationWindow)
		
		GemRB.UnhideGUI()
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)
        GemRB.SetVar ("FloatWindow", BiographyWindow)

	
	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	#GemRB.SetVisible (Window, 1)
	GemRB.UnhideGUI ()
	

###################################################
# End of file GUIREC.py
