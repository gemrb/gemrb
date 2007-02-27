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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack

# GUIREC:
# 0,1,2 - common windows (time, message, menu)
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
from GUICommon import CloseOtherWindow
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler, RunSelectionChangeHandler
from GUIWORLD import OpenReformPartyWindow

###################################################
LevelUpWindow = None
RecordsWindow = None
InformationWindow = None
BiographyWindow = None
statevents =("OnRecordsHelpStrength","OnRecordsHelpIntelligence","OnRecordsHelpWisdom","OnRecordsHelpDexterity","OnRecordsHelpConstitution","OnRecordsHelpCharisma")

###################################################
def OpenRecordsWindow ():
	global RecordsWindow
	global StatTable

	StatTable = GemRB.LoadTable("abcomm")
	
	if CloseOtherWindow (OpenRecordsWindow):
		GemRB.HideGUI ()
		if InformationWindow: OpenInformationWindow ()
		
		GemRB.UnloadWindow (RecordsWindow)
		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		SetSelectionChangeHandler (None)

		GemRB.UnhideGUI ()
		return	

	GemRB.HideGUI ()
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
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLevelUpWindow")

	# stat buttons
	for i in range (6):
		Button = GemRB.GetControl (Window, 31 + i)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		GemRB.SetButtonSprites(Window, Button, "", 0, 0, 0, 0, 0)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, statevents[i])
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")

	# AC button
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonSprites(Window, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, "OnRecordsHelpArmorClass")
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")


	# HP button
	Button = GemRB.GetControl (Window, 38)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonSprites (Window, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, "OnRecordsHelpHitPoints")
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")


	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()

	GemRB.UnhideGUI ()


stats_overview = None
faction_help = ''
alignment_help = ''
avatar_header = {'PrimClass': "", 'SecoClass': "", 'PrimLevel': 0, 'SecoLevel': 0, 'XP': 0, 'PrimNextLevXP': 0, 'SecoNextLevXP': 0}

def UpdateRecordsWindow ():
	global stats_overview, faction_help, alignment_help
	
	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	
	# Setting up the character information
	GetCharacterHeader (pc)

	# Checking whether character has leveled up.
	Button = GemRB.GetControl (Window, 9)
	if avatar_header['SecoLevel'] == 0:
		if avatar_header['XP'] >= avatar_header['PrimNextLevXP']:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		# Character is Multi-Class
		if avatar_header['XP'] >= avatar_header['PrimNextLevXP'] or avatar_header['XP'] >= avatar_header['SecoNextLevXP']:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# name
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))

	# portrait
	Image = GemRB.GetControl (Window, 2)
	GemRB.SetButtonState (Window, Image, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Image, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture (Window, Image, GetActorPortrait (pc, 'STATS'))

	# armorclass
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))
	GemRB.SetTooltip (Window, Label, 4197)

	# hp now
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
	GemRB.SetTooltip (Window, Label, 4199)

	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	bstr = GemRB.GetPlayerStat (pc, IE_STR,1)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	bstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA,1)
	if (sstrx > 0) and (sstr==18):
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	if (bstrx > 0) and (bstr==18):
		bstr = "%d/%02d" %(bstr, bstrx % 100)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	bint = GemRB.GetPlayerStat (pc, IE_INT,1)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	bwis = GemRB.GetPlayerStat (pc, IE_WIS,1)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	bdex = GemRB.GetPlayerStat (pc, IE_DEX,1)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	bcon = GemRB.GetPlayerStat (pc, IE_CON,1)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	bchr = GemRB.GetPlayerStat (pc, IE_CHR,1)

	stats = (sstr, sint, swis, sdex, scon, schr)
	basestats = (bstr, bint, bwis, bdex, bcon, bchr)
	for i in range (6):
		Label = GemRB.GetControl (Window, 0x1000000e + i)
		if stats[i]!=basestats[i]:
			GemRB.SetLabelTextColor (Window, Label, 255, 0, 0)
		else:
			GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)
		GemRB.SetText (Window, Label, str (stats[i]))

	# race
	# FIXME: for some strange reason, Morte's race is 1 (Human)
	# in the save files, instead of 45 (Morte)
	RaceTable = GemRB.LoadTable ("RACES")
	print "species: %d  race: %d" %(GemRB.GetPlayerStat (pc, IE_SPECIES), GemRB.GetPlayerStat (pc, IE_RACE))
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


	# class
	ClassTable = GemRB.LoadTable ("classes")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x10000016)
	GemRB.SetText (Window, Label, text)

	# alignment
	align = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	ss = GemRB.LoadSymbol ("ALIGN")
	sym = GemRB.GetSymbolValue (ss, align)

	AlignmentTable = GemRB.LoadTable ("ALIGNS")
	alignment_help = GemRB.GetString (GemRB.GetTableValue (AlignmentTable, sym, 'DESC_REF'))
	frame = (3 * int (align / 16) + align % 16) - 4
	
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonSprites (Window, Button, 'STALIGN', 0, frame, 0, 0, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_OVER_BUTTON, "OnRecordsHelpAlignment")
	GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "OnRecordsButtonLeave")


	# faction
	faction = GemRB.GetPlayerStat (pc, IE_FACTION)
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
	return

# puts default info to textarea (overview of PC's bonuses, saves, etc.
def OnRecordsButtonLeave ():
	Window = RecordsWindow
	# help, info textarea
	Text = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Text, stats_overview)
	return

def OnRecordsHelpFaction ():
	Window = RecordsWindow
	#Help = GemRB.GetVar ("ControlHelp")
	Help = GemRB.GetString (20106) + "\n\n" + faction_help
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	return
	
	# 20106 faction

	# 34584 no align 1/8
	# 34588 Dustmen 4/8 
	# 34585 godsmen 8/8
	# 34587 Sensates 3/8
	# 34589 Anarchists 5/8
	# 34590 Xaositects 6/8
	# 3789 indeps 7/8
	# 34586 Mercykillers 2/8

def OnRecordsHelpArmorClass ():
	Window = RecordsWindow
	Help = GemRB.GetString (18493)
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	return

def OnRecordsHelpHitPoints ():
	Window = RecordsWindow
	Help = GemRB.GetString (18494)
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	return

def OnRecordsHelpAlignment ():
	Window = RecordsWindow
	Help = GemRB.GetString (20105) + "\n\n" + alignment_help
	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, Help)
	return
	
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

#Bio:
# 38787 no
# 39423 morte
# 39424 annah
# 39425 dakkon
# 39426 ffg
# 39427 ignus
# 39428 nordom
# 39429 vhailor

def OnRecordsHelpStrength ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# Loading tables of modifications
	Table = GemRB.LoadTable("strmod")
	TableEx = GemRB.LoadTable("strmodex")

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's strength
	s = GemRB.GetPlayerStat (pc, IE_STR)
	e = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	x = GemRB.GetTableValue(Table, s, 0) + GemRB.GetTableValue(TableEx, e, 0)
	y = GemRB.GetTableValue(Table, s, 1) + GemRB.GetTableValue(TableEx, e, 1)
	if x==0:
		x=y
		y=0
	if e>60:
		s=19
	GemRB.SetText(Window, TextArea, 18489)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,s,0),x,y) )

	# Unloading tables
	#GemRB.UnloadTable (Table)
	#GemRB.UnloadTable (TableEx)
	return

def OnRecordsHelpDexterity ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# Loading table of modifications
	Table = GemRB.LoadTable("dexmod")

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's dexterity
	Dex = GemRB.GetPlayerStat (pc, IE_DEX)

	# Getting the dexterity description
	x = -GemRB.GetTableValue(Table,Dex,2)

	GemRB.SetText(Window, TextArea, 18487)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Dex,3),x,0) )
	return

def OnRecordsHelpIntelligence ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's intelligence
	Int = GemRB.GetPlayerStat (pc, IE_INT)

	GemRB.SetText(Window, TextArea, 18488)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Int,1),0,0) )
	return

def OnRecordsHelpWisdom ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's wisdom
	Wis = GemRB.GetPlayerStat (pc, IE_WIS)

	GemRB.SetText(Window, TextArea, 18490)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Wis,2),0,0) )
	return

def OnRecordsHelpConstitution ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# Loading table of modifications
	Table = GemRB.LoadTable("hpconbon")

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's constitution
	Con = GemRB.GetPlayerStat (pc, IE_CON)

	# Getting the constitution description
	x = GemRB.GetTableValue(Table,Con-1,1)

	GemRB.SetText(Window, TextArea, 18491)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Con,4),x,0) )
	return

def OnRecordsHelpCharisma ():
	Window = RecordsWindow
	TextArea = GemRB.GetControl (Window, 0)

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's charisma
	Cha = GemRB.GetPlayerStat (pc, IE_CHR)

	GemRB.SetText(Window, TextArea, 1903)
	GemRB.TextAreaAppend(Window, TextArea, "\n\n"+GemRB.StatComment(GemRB.GetTableValue(StatTable,Cha,5),0,0) )
	return

def GetCharacterHeader (pc):
	global avatar_header

	ClassTable = GemRB.LoadTable ("classes")
	BioTable = GemRB.LoadTable ("bios")

	Class = GemRB.GetPlayerStat (pc, IE_CLASS) - 1
	Multi = GemRB.GetTableValue (ClassTable, Class, 4)
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)

	#Nameless is Specific == 1
	avatar_header['Specific'] = Specific

	# Nameless is a special case (dual class)
	if Specific == 1:
		avatar_header['PrimClass'] = GemRB.GetTableRowName (ClassTable, Class)
		avatar_header['SecoClass'] = "*"

		avatar_header['SecoLevel'] = 0

		if avatar_header['PrimClass'] == "FIGHTER":
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP)
		elif avatar_header['PrimClass'] == "MAGE":
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL2)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP_MAGE)
		else:
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL3)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP_THIEF)

		avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
		avatar_header['SecoNextLevXP'] = 0
	else:
		# PC is not NAMELESS_ONE
		avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL)
		avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP)
		if Multi:
			avatar_header['XP'] = avatar_header['XP'] / 2
			avatar_header['SecoLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL2)

			avatar_header['PrimClass'] = "FIGHTER"
			if Multi == 3:
				#fighter/mage
				Class = 0
			else:
				#fighter/thief
				Class = 3
			avatar_header['SecoClass'] = GemRB.GetTableRowName (ClassTable, Class)

			avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
			avatar_header['SecoNextLevXP'] = GetNextLevelExp (avatar_header['SecoLevel'], avatar_header['SecoClass'])

			# Converting to the displayable format
			avatar_header['SecoClass'] = GemRB.GetString (GemRB.GetTableValue (ClassTable, avatar_header['SecoClass'], "NAME_REF"))
		else:
			avatar_header['SecoLevel'] = 0
			avatar_header['PrimClass'] = GemRB.GetTableRowName (ClassTable, Class)
			avatar_header['SecoClass'] = "*"
			avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
			avatar_header['SecoNextLevXP'] = 0

	# Converting to the displayable format
	avatar_header['PrimClass'] = GemRB.GetString (GemRB.GetTableValue (ClassTable, avatar_header['PrimClass'], "NAME_REF"))

	GemRB.UnloadTable (ClassTable)
	GemRB.UnloadTable (BioTable)


def GetNextLevelExp (Level, Class):
	NextLevelTable = GemRB.LoadTable ("XPLEVEL")

	if (Level < 20):
		NextLevel = GemRB.GetTableValue (NextLevelTable, Class, str (Level + 1))
	else:
		After21ExpTable = GemRB.LoadTable ("LVL21PLS")
		ExpGap = GemRB.GetTableValue (After21ExpTable, Class, 'XPGAP')
		LevDiff = Level - 19
		Lev20Exp = GemRB.GetTableValue (NextLevelTable, Class, "20")
		NextLevel = Lev20Exp + (LevDiff * ExpGap)

	return NextLevel


def GetStatOverview (pc):
	won = "[color=FFFFFF]"
	woff = "[/color]"
	str_None = GemRB.GetString (41275)
	
	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)

	stats = []

	# Displaying Class, Level, Experience and Next Level Experience
	# FIXME: This looks ugly
	if (avatar_header['SecoLevel'] == 0):
		DispLevel = GemRB.GetString (48156) + ': ' + str (avatar_header['PrimLevel']) + "\n"
		DispXP = GemRB.GetString (19673) + ': ' + str (avatar_header['XP']) + "\n"
		DispNextLevel = GemRB.GetString (19674) + ': ' + str (avatar_header['PrimNextLevXP']) + "\n"
		Main = avatar_header['PrimClass'] + "\n" + DispLevel + DispXP + DispNextLevel + "\n"
	else:
		DispPrimClass = avatar_header['PrimClass'] + "\n"
		DispPrimLevel = GemRB.GetString (48156) + ': ' + str (avatar_header['PrimLevel']) + "\n"
		DispXP = GemRB.GetString (19673) + ': ' + str (avatar_header['XP']) + "\n"
		DispPrimNextLevel = GemRB.GetString (19674) + ': ' + str (avatar_header['PrimNextLevXP']) + "\n"

		DispSecoClass = avatar_header['SecoClass'] + "\n"
		DispSecoLevel = GemRB.GetString (48156) + ': ' + str (avatar_header['SecoLevel']) + "\n"
		DispSecoNextLevel = GemRB.GetString (19674) + ': ' + str (avatar_header['SecoNextLevXP']) + "\n"
		Main = GemRB.GetString (19414) + "\n\n" + DispPrimClass + DispPrimLevel + DispXP + DispPrimNextLevel + "\n"
		Main = Main + DispSecoClass + DispSecoLevel + DispXP + DispSecoNextLevel + "\n"

	# 59856 Current State
	StatesTable = GemRB.LoadTable ("states")
	StateID = GS (IE_STATE_ID)
	State = GemRB.GetString (GemRB.GetTableValue (StatesTable, str (StateID), "NAME_REF"))
	CurrentState = won + GemRB.GetString (59856) + woff + "\n" + State + "\n\n"
	GemRB.UnloadTable (StatesTable)

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
	tmp = GS (IE_NUMBEROFATTACKS)
	if (tmp&1):
		tmp2 = str(tmp/2) + chr(188)
	else:
		tmp2 = str(tmp/2)

	stats.append ((4209, tmp2, ''))
	#   4210 Lore
	stats.append ((4210, GS (IE_LORE), ''))
	#   4211 Open Locks
	stats.append ((4211, GS (IE_LOCKPICKING), '%'))
	#   4212 Stealth
	stats.append ((4212, GS (IE_STEALTH), '%'))
	#   4213 Find/Remove Traps
	stats.append ((4213, GS (IE_TRAPS), '%'))
	#   4214 Pick Pockets
	stats.append ((4214, GS (IE_PICKPOCKET), '%'))
	#   4215 Tracking
	stats.append ((4215, GS (IE_TRACKING), ''))
	#   4216 Reputation
	stats.append ((4216, GS (IE_REPUTATION), ''))
	#   4217 Turn Undead Level
	stats.append ((4217, GS (IE_TURNUNDEADLEVEL), ''))
	#   4218 Lay on Hands Amount
	stats.append ((4218, GS (IE_LAYONHANDSAMOUNT), ''))
	#   4219 Backstab Damage
	stats.append ((4219, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x'))
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
	stats.append ((55011, GS (IE_FREESLOTS), ''))
	#   33642 Fist
	stats.append ((33642, GS (IE_PROFICIENCYBASTARDSWORD), '+'))
	#   33649 Edged Weapon
	stats.append ((33649, GS (IE_PROFICIENCYLONGSWORD), '+'))
	#   33651 Hammer
	stats.append ((33651, GS (IE_PROFICIENCYSHORTSWORD), '+'))
	#   44990 Axe
	stats.append ((44990, GS (IE_PROFICIENCYAXE), '+'))
	#   33653 Club
	stats.append ((33653, GS (IE_PROFICIENCYTWOHANDEDSWORD), '+'))
	#   33655 Bow
	stats.append ((33655, GS (IE_PROFICIENCYKATANA), '+'))
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
			if type == '+':
				res.append (GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'x':
				res.append (GemRB.GetString (strref) + ': x' + str (val))
			else:
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


	TotalPartyExp = 0
	TotalPartyKills = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsChapterXP']
		TotalPartyKills = TotalPartyKills + stat['KillsChapterCount']

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	stat = GemRB.GetPCStats (pc)

	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))


	# class
	ClassTable = GemRB.LoadTable ("classes")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x1000000A)
	GemRB.SetText (Window, Label, text)



	Label = GemRB.GetControl (Window, 0x10000002)
	if stat['BestKilledName'] == -1:
		GemRB.SetText (Window, Label, GemRB.GetString (41275))
	else:
		GemRB.SetText (Window, Label, GemRB.GetString (stat['BestKilledName']))

	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates???). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15) 
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))
	
	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, 41277)

	Label = GemRB.GetControl (Window, 0x10000004)
	GemRB.SetText (Window, Label, stat['FavouriteSpell'])

	Label = GemRB.GetControl (Window, 0x10000005)
	GemRB.SetText (Window, Label, stat['FavouriteWeapon'])

	Label = GemRB.GetControl (Window, 0x10000006)
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / TotalPartyExp)
		GemRB.SetText (Window, Label, str (PartyExp) + '%')
	else:
		GemRB.SetText (Window, Label, "0%")

	Label = GemRB.GetControl (Window, 0x10000007)
	if TotalPartyKills != 0:
		PartyKills = int ((stat['KillsChapterCount'] * 100) / TotalPartyKills)
		GemRB.SetText (Window, Label, str (PartyKills) + '%')
	else:
		GemRB.SetText (Window, Label, '0%')

	Label = GemRB.GetControl (Window, 0x10000008)
	GemRB.SetText (Window, Label, str (stat['KillsTotalXP']))

	Label = GemRB.GetControl (Window, 0x10000009)
	GemRB.SetText (Window, Label, str (stat['KillsTotalCount']))


	Label = GemRB.GetControl (Window, 0x1000000B)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x1000000C)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x1000000D)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x1000000E)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x1000000F)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x10000010)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x10000011)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

	Label = GemRB.GetControl (Window, 0x10000012)
	GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)



	# Biography
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4247)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)



def OpenBiographyWindow ():
	global BiographyWindow

	GemRB.HideGUI ()
	
	if BiographyWindow != None:
		GemRB.UnloadWindow (BiographyWindow)
		BiographyWindow = None
		GemRB.SetVar ("FloatWindow", InformationWindow)
		
		GemRB.UnhideGUI()
		GemRB.ShowModal (InformationWindow, MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)
	GemRB.SetVar ("FloatWindow", BiographyWindow)

	# These are used to get the bio
	pc = GemRB.GameGetSelectedPCSingle ()

	BioTable = GemRB.LoadTable ("bios")
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	BioText = int (GemRB.GetTableValue (BioTable, Specific, 'BIO'))
	GemRB.UnloadTable (BioTable)

	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, BioText)

	
	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	

def AcceptLevelUp():
	OpenLevelUpWindow()
	#do level up
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH, SavThrows[0])
	GemRB.SetPlayerStat (pc, IE_SAVEVSWANDS, SavThrows[1])
	GemRB.SetPlayerStat (pc, IE_SAVEVSPOLY, SavThrows[2])
	GemRB.SetPlayerStat (pc, IE_SAVEVSBREATH, SavThrows[3])
	GemRB.SetPlayerStat (pc, IE_SAVEVSSPELL, SavThrows[4])
	oldhp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, HPGained+oldhp)
	oldhp = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, HPGained+oldhp)
	#increase weapon proficiency if needed
	if WeapProfType!=-1:
		GemRB.SetPlayerStat (pc, WeapProfType, CurrWeapProf + WeapProfGained )
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	if Specific == 1:
		#TODO:
		#the nameless one is dual classed
		#so we have to determine which level to increase
		GemRB.SetPlayerStat (pc, IE_LEVEL, GemRB.GetPlayerStat (pc, IE_LEVEL)+NumOfPrimLevUp)
	else:
		GemRB.SetPlayerStat (pc, IE_LEVEL, GemRB.GetPlayerStat (pc, IE_LEVEL)+NumOfPrimLevUp)
		GemRB.SetPlayerStat (pc, IE_LEVEL2, GemRB.GetPlayerStat (pc, IE_LEVEL2)+NumOfSecoLevUp)
	UpdateRecordsWindow ()


def OpenLevelUpWindow ():
	global LevelUpWindow
	global SavThrows
	global HPGained
	global WeapProfType, CurrWeapProf, WeapProfGained
	global NumOfPrimLevUp, NumOfSecoLevUp

	GemRB.HideGUI ()

	if LevelUpWindow != None:
		GemRB.UnloadWindow (LevelUpWindow)
		LevelUpWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI()
		return

	LevelUpWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", LevelUpWindow)

	# Accept
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 4192)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "AcceptLevelUp")

	pc = GemRB.GameGetSelectedPCSingle ()
	ClassTable = GemRB.LoadTable ("classes")

	# These are used to identify Nameless One
	BioTable = GemRB.LoadTable ("bios")
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = GemRB.GetTableValue (BioTable, Specific, "PC")

	# These will be used for saving throws
	SavThrUpdated = False
	SavThrows = [0,0,0,0,0]
	SavThrows[0] = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH)
	SavThrows[1] = GemRB.GetPlayerStat (pc, IE_SAVEVSWANDS)
	SavThrows[2] = GemRB.GetPlayerStat (pc, IE_SAVEVSPOLY)
	SavThrows[3] = GemRB.GetPlayerStat (pc, IE_SAVEVSBREATH)
	SavThrows[4] = GemRB.GetPlayerStat (pc, IE_SAVEVSSPELL)

	HPGained = 0
	ConHPBon = 0
	Thac0Updated = False
	Thac0 = 0
	WeapProfGained = 0

	ClasWeapTable = GemRB.LoadTable ("weapprof")
	WeapProfType = -1
	CurrWeapProf = -1
	#This does not apply to Nameless since he uses unused slots system
	#Nameless is Specific == 1
	if Specific != "1":
		# Searching for the column name where value is 1
		for i in range (5):
			WeapProfName = GemRB.GetTableRowName (ClasWeapTable, i)
			value = GemRB.GetTableValue (ClasWeapTable, AvatarName, WeapProfName)
			if value == 1:
				WeapProfType = i
				break

	if WeapProfType!=-1:
		CurrWeapProf = GemRB.GetPlayerStat (pc, IE_WEAPPROF+WeapProfType)

	# Recording this avatar's current proficiency level
	# Since Nameless one is not covered, hammer and club can't occur
	# What is the avatar's class (Which we can use to lookup XP)
	ClassRow = GemRB.GetPlayerStat (pc, IE_CLASS)-1
	Class = GemRB.GetTableRowName (ClassTable, ClassRow)

	# name
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))

	# class
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, GemRB.GetTableValue (ClassTable, ClassRow, 0))

	# Armor Class
	Label = GemRB.GetControl (Window, 0x10000023)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))

	# Thief Skills
	# For now we shall set them to the current levels and disable the increment buttons
	# Points Left
	Label = GemRB.GetControl (Window, 0x10000006)
	GemRB.SetText (Window, Label, '0')
	# Stealth
	Label = GemRB.GetControl (Window, 0x10000008)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_STEALTH)) + '%')
	# Detect Traps
	Label = GemRB.GetControl (Window, 0x1000000A)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_TRAPS)) + '%')
	# Pick Pockets
	Label = GemRB.GetControl (Window, 0x1000000C)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_PICKPOCKET)) + '%')
	# Open Doors
	Label = GemRB.GetControl (Window, 0x1000000E)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_LOCKPICKING)) + '%')
	# Plus and Minus buttons
	for i in range (8):
		Button = GemRB.GetControl (Window, 16 + i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Is avatar multi-class?
	if avatar_header['SecoLevel'] == 0:
		# avatar is single class
		# What will be avatar's next level?
		NextLevel = avatar_header['PrimLevel'] + 1
		while avatar_header['XP'] >= GetNextLevelExp (NextLevel, Class):
			NextLevel = NextLevel + 1
		NumOfPrimLevUp = NextLevel - avatar_header['PrimLevel'] # How many levels did we go up?

		# Is avatar Nameless One?
		if Specific == "1":
			# Saving Throws
			# Nameless One gets the best possible throws from all the classes except Priest
			FigSavThrTable = GemRB.LoadTable ("SAVEWAR")
			MagSavThrTable = GemRB.LoadTable ("SAVEWIZ")
			ThiSavThrTable = GemRB.LoadTable ("SAVEROG")
			# For Nameless, we also need to know his levels in every class, so that we pick
			# the right values from the corresponding tables. Also we substract one, so
			# that we may use them as column indices in tables.
			FighterLevel = GemRB.GetPlayerStat (pc, IE_LEVEL) - 1
			MageLevel = GemRB.GetPlayerStat (pc, IE_LEVEL2) - 1
			ThiefLevel = GemRB.GetPlayerStat (pc, IE_LEVEL3) - 1
			# this is the constitution bonus type for this level
			CONType = 1
			# We are leveling up one of those levels. Therefore, one of them has to be updated.
			if avatar_header['PrimClass'] == "FIGHTER":
				FighterLevel = NextLevel - 1
				CONType = 0
			elif avatar_header['PrimClass'] == "MAGE":
				MageLevel = NextLevel - 1
			else:
				ThiefLevel = NextLevel - 1

			ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, 0, CONType)
			# Now we need to update the saving throws with the best values from those tables.
			# The smaller the number, the better saving throw it is.
			# We also need to check if any of the levels are larger than 21, since
			# after that point the table runs out, and the throws remain the
			# same
			if FighterLevel < 21:
				for i in range (5):
					Throw = GemRB.GetTableValue (FigSavThrTable, i, FighterLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			if MageLevel < 21:
				for i in range (5):
					Throw = GemRB.GetTableValue (MagSavThrTable, i, MageLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			if ThiefLevel < 21:
				for i in range (5):
					Throw = GemRB.GetTableValue (ThiSavThrTable, i, ThiefLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			# Cleaning up
			GemRB.UnloadTable (FigSavThrTable)
			GemRB.UnloadTable (MagSavThrTable)
			GemRB.UnloadTable (ThiSavThrTable)
		else:
			#How many weapon procifiencies we get
			for i in range (NumOfPrimLevUp):
				if HasGainedWeapProf (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, avatar_header['PrimClass']):
					WeapProfGained += 1

			# Saving Throws
			# Loading the right saving throw table
			SavThrTable = GemRB.LoadTable (GemRB.GetTableValue (ClassTable, Class, "SAVE"))
			# Updating the current saving throws. They are changed only if the
			# new ones are better than current. The smaller the number, the better.
			# We need to substract one from the NextLevel, so that we get right values.
			# We also need to check if NextLevel is larger than 21, since after that point
			# the table runs out, and the throws remain the same 
			if NextLevel < 22:
				for i in range (5):
					Throw = GemRB.GetTableValue (SavThrTable, i, NextLevel-1)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			# Cleaning Up
			GemRB.UnloadTable (SavThrTable)

			# Hit Points Gained and Hit Points from Constitution Bonus
			for i in range (NumOfPrimLevUp):
				HPGained = HPGained + GetSingleClassHP (Class, avatar_header['PrimLevel'])

			if avatar_header['PrimClass'] == "FIGHTER":
				CONType = 0
			else:
				CONType = 1
			ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, 0, CONType)

			# Thac0
			Thac0 = GetThac0 (Class, NextLevel)
			# Is the new thac0 better than old? (The smaller, the better)
			if Thac0 < GemRB.GetPlayerStat (pc, IE_THAC0):
				Thac0Updated = True

	else:
		# avatar is multi class
		# we have only fighter/X multiclasses, so this
		# part is a bit hardcoded
		PrimNextLevel = 0
		SecoNextLevel = 0
		NumOfPrimLevUp = 0
		NumOfSecoLevUp = 0

		# What will be avatar's next levels?
		PrimNextLevel = avatar_header['PrimLevel']
		while avatar_header['XP'] >= GetNextLevelExp (PrimNextLevel, "FIGHTER"):
			PrimNextLevel = PrimNextLevel + 1
		# How many primary levels did we go up?
		NumOfPrimLevUp = PrimNextLevel - avatar_header['PrimLevel']

		for i in range (NumOfPrimLevUp):
			if HasGainedWeapProf (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, avatar_header['PrimClass']):
				WeapProfGained += 1

		# Saving Throws
		FigSavThrTable = GemRB.LoadTable ("SAVEWAR")
		if PrimNextLevel < 22:
			for i in range (5):
				Throw = GemRB.GetTableValue (FigSavThrTable, i, PrimNextLevel - 1)
				if Throw < SavThrows[i]:
					SavThrows[i] = Throw
					SavThrUpdated = True
		GemRB.UnloadTable (FigSavThrTable)
		# Which multi class is it?
		if GemRB.GetPlayerStat (pc, IE_CLASS) == 7:
			# avatar is Fighter/Mage (Dak'kon)
			Class = "MAGE"
			SavThrTable = GemRB.LoadTable ("SAVEWIZ")
		else:
			# avatar is Fighter/Thief (Annah)
			Class = "THIEF"
			SavThrTable = GemRB.LoadTable ("SAVEROG")
		
		SecoNextLevel = avatar_header['SecoLevel']
		while avatar_header['XP'] >= GetNextLevelExp (SecoNextLevel, Class):
			SecoNextLevel = SecoNextLevel + 1
		# How many secondary levels did we go up?
		NumOfSecoLevUp = SecoNextLevel - avatar_header['SecoLevel']
		if SecoNextLevel < 22:
			for i in range (5):
				Throw = GemRB.GetTableValue (SavThrTable, i, SecoNextLevel - 1)
				if Throw < SavThrows[i]:
					SavThrows[i] = Throw
					SavThrUpdated = True
		GemRB.UnloadTable (SavThrTable)

		# Hit Points Gained and Hit Points from Constitution Bonus (multiclass)
		for i in range (NumOfPrimLevUp):
			HPGained = HPGained + GetSingleClassHP ("FIGHTER", avatar_header['PrimLevel'])/2

		for i in range (NumOfSecoLevUp):
			HPGained = HPGained + GetSingleClassHP (Class, avatar_header['SecoLevel'])/2
		ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, NumOfSecoLevUp, 2)

		# Thac0
		# Multi class use the primary class level to determine Thac0
		Thac0 = GetThac0 (Class, PrimNextLevel)
		# Is the new thac0 better than old? (The smaller the better)
		if Thac0 < GemRB.GetPlayerStat (pc, IE_THAC0):
			Thac0Updated = True


	# Displaying the saving throws
	# Death
	Label = GemRB.GetControl (Window, 0x10000019)
	GemRB.SetText (Window, Label, str (SavThrows[0]))
	# Wand
	Label = GemRB.GetControl (Window, 0x1000001B)
	GemRB.SetText (Window, Label, str (SavThrows[1]))
	# Polymorph
	Label = GemRB.GetControl (Window, 0x1000001D)
	GemRB.SetText (Window, Label, str (SavThrows[2]))
	# Breath
	Label = GemRB.GetControl (Window, 0x1000001F)
	GemRB.SetText (Window, Label, str (SavThrows[3]))
	# Spell
	Label = GemRB.GetControl (Window, 0x10000021)
	GemRB.SetText (Window, Label, str (SavThrows[4]))

	FinalCurHP = GemRB.GetPlayerStat (pc, IE_HITPOINTS) + HPGained
	FinalMaxHP = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS) + HPGained

	# Current HP
	Label = GemRB.GetControl (Window, 0x10000025)
	GemRB.SetText (Window, Label, str (FinalCurHP))

	# Max HP
	Label = GemRB.GetControl (Window, 0x10000027)
	GemRB.SetText (Window, Label, str (FinalMaxHP))

	# Displaying level up info
	overview = ""
	if CurrWeapProf!=-1 and WeapProfGained>0:
		overview = overview + '+' + str (WeapProfGained) + ' ' + GemRB.GetString (WeapProfDispStr) + '\n'

	overview = overview + str (HPGained) + " " + GemRB.GetString (38713) + '\n'
	overview = overview + str (ConHPBon) + " " + GemRB.GetString (38727) + '\n'

	if SavThrUpdated:
		overview = overview + GemRB.GetString (38719) + '\n'
	if Thac0Updated:
		overview = overview + GemRB.GetString (38718) + '\n'

	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, overview)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

def GetSingleClassHP (Class, Level):
	ClassTable = GemRB.LoadTable ("classes")
	HPTable = GemRB.LoadTable (GemRB.GetTableValue (ClassTable, Class, "HP"))

	# We need to check if Level is larger than 20, since after that point
	# the table runs out, and the formula remain the same.
	if Level > 20:
		Level = 20

	# We need the Level as a string, so that we can use the column names
	Level = str (Level)

	Sides = GemRB.GetTableValue (HPTable, Level, "SIDES")
	Rolls = GemRB.GetTableValue (HPTable, Level, "ROLLS")
	Modif = GemRB.GetTableValue (HPTable, Level, "MODIFIER")

	GemRB.UnloadTable (HPTable)

	return GemRB.Roll (Rolls, Sides, Modif)

def GetConHPBonus (pc, numPrimLevels, numSecoLevels, levelUpType):
	ConHPBonTable = GemRB.LoadTable ("HPCONBON")

	con = str (GemRB.GetPlayerStat (pc, IE_CON))
	if levelUpType == 0:
		# Pure fighter
		return GemRB.GetTableValue (ConHPBonTable, con, "WARRIOR") * numPrimLevels
	if levelUpType == 1:
		# Mage, Priest or Thief
		return GemRB.GetTableValue (ConHPBonTable, con, "OTHER") * numPrimLevels
	return GemRB.GetTableValue (ConHPBonTable, con, "WARRIOR") * numPrimLevels / 2 + GemRB.GetTableValue (ConHPBonTable, con, "OTHER") * numSecoLevels / 2

def GetThac0 (Class, Level):
	Thac0Table = GemRB.LoadTable ("THAC0")
	# We need to check if Level is larger than 60, since after that point
	# the table runs out, and the value remains the same.
	if Level > 60:
		Level = 60

	return GemRB.GetTableValue (Thac0Table, Class, str (Level))

#apparently the original code doesn't follow profsmax/profs tables
def HasGainedWeapProf (pc, currProf, currLevel, Class):
	#only fighters gain weapon proficiencies
	if Class!="FIGHTER":
		return False
	#hardcoded limit is 4
	if currProf>3:
		return False
	if CurrProf>(currLevel-1)/3:
		return False
	return True

###################################################
# End of file GUIREC.py
