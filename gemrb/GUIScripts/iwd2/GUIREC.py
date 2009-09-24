# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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
# $Id: GUIREC.py 4503 2007-02-27 16:43:46Z wjpalenstijn $


# GUIREC.py - scripts to control character record windows from GUIREC winpack

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow, SetColorStat
from GUICommonWindows import *

SelectWindow = 0
Topic = None
HelpTable = None
DescTable = None
RecordsWindow = None
RecordsTextArea = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None

def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow, SelectWindow

	if CloseOtherWindow (OpenRecordsWindow):
		if RecordsWindow:
			RecordsWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIREC", 800, 600)
	RecordsWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenRecordsWindow")
	Window.SetFrame ()

	#portrait icon
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	#information (help files)
	Button = Window.GetControl (1)
	Button.SetText (11946)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenHelpWindow")

	#biography
	Button = Window.GetControl (59)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	#export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenExportWindow")

	#customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenCustomizeWindow")

	#general
	GemRB.SetVar ("SelectWindow", 1)

	Button = Window.GetControl (60)
	Button.SetTooltip (40316)
	Button.SetVarAssoc ("SelectWindow", 1)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#weapons and armour
	Button = Window.GetControl (61)
	Button.SetTooltip (40317)
	Button.SetVarAssoc ("SelectWindow", 2)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#skills and feats
	Button = Window.GetControl (62)
	Button.SetTooltip (40318)
	Button.SetVarAssoc ("SelectWindow", 3)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#miscellaneous
	Button = Window.GetControl (63)
	Button.SetTooltip (33500)
	Button.SetVarAssoc ("SelectWindow", 4)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#level up
	Button = Window.GetControl (37)
	Button.SetText (7175)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LevelUpWindow")

	SetSelectionChangeHandler (RefreshRecordsWindow)

	RefreshRecordsWindow ()
	return

def ColorDiff (Window, Label, diff):
	if diff>0:
		Label.SetTextColor (0, 255, 0)
	elif diff<0:
		Label.SetTextColor (255, 0, 0)
	else:
		Label.SetTextColor (255, 255, 255)
	return

def ColorDiff2 (Window, Label, diff):
	if diff:
		Label.SetTextColor (255, 255, 0)
	else:
		Label.SetTextColor (255, 255, 255)
	return

def HasBonusSpells (pc):
	return False

def HasClassFeatures (pc):
	#clerics turning
	if GemRB.GetPlayerStat(pc, IE_TURNUNDEADLEVEL):
		return True
	#thieves backstabbing damage
	if GemRB.GetPlayerStat(pc, IE_BACKSTABDAMAGEMULTIPLIER):
		return True
	#paladins healing
	if GemRB.GetPlayerStat(pc, IE_LAYONHANDSAMOUNT):
		return True
	return False

def GetFavoredClass (pc, code):
	if GemRB.GetPlayerStat (pc, IE_SEX)==1:
		code = code&15
	else:
		code = (code>>8)&15

	return code-1

def GetAbilityBonus (pc, stat):
	Ability = GemRB.GetPlayerStat (pc, stat)
	return Ability//2-5

#class is ignored
def GetNextLevelExp (Level, Adjustment):
	NextLevelTable = GemRB.LoadTableObject ("XPLEVEL")

	if Adjustment>5:
		Adjustment = 5
	if (Level < NextLevelTable.GetColumnCount (4) - 5):
		return str(NextLevelTable.GetValue (4, Level + Adjustment ) )

	return GemRB.GetString(24342) #godhood

#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = [IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
IE_LEVELSORCEROR, IE_LEVEL2]

def DisplayGeneral (pc):
	Window = RecordsWindow

	#levels
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (40308)
	RecordsTextArea.Append (" - ")
	RecordsTextArea.Append (40309)
	levelsum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	#TODO: get special level penalty for subrace
	adj = 0
	RecordsTextArea.Append (": "+str(levelsum) )
	RecordsTextArea.Append ("[/color]")
	#the class name for highest
	highest = None
	tmp = 0
	for i in range(11):
		level = GemRB.GetPlayerStat (pc, Classes[i])

		if level:
			Class = GetActorClassTitle (pc, i )
			RecordsTextArea.Append (Class, -1)
			RecordsTextArea.Append (": "+str(level) )
			if tmp<level:
				highest = i
				tmp = level

	RecordsTextArea.Append ("\n")
	#effective character level
	if adj:
		RecordsTextArea.Append (40311,-1)
		RecordsTextArea.Append (": "+str(levelsum+adj) )

	#favoured class
	RecordsTextArea.Append (40310,-1)
	RaceTable = GemRB.LoadTableObject("races")
	ClassTable = GemRB.LoadTableObject("classes")
	AlignTable = GemRB.LoadTableObject("aligns")

	#get the subrace value
	Value = GemRB.GetPlayerStat(pc,IE_RACE)
	Value2 = GemRB.GetPlayerStat(pc,IE_SUBRACE)
	if Value2:
		Value = Value<<16 | Value2
	tmp = RaceTable.FindValue (3, Value)
	Race = RaceTable.GetValue (tmp, 2)
	tmp = RaceTable.GetValue (tmp, 8)

	if tmp == -1:
		tmp = highest
	else:
		tmp = GetFavoredClass(pc, tmp)

	tmp = ClassTable.GetValue (tmp, 0)
	RecordsTextArea.Append (": ")
	RecordsTextArea.Append (tmp)

	#experience
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (17089)
	RecordsTextArea.Append ("[/color]")

	RecordsTextArea.Append (36928,-1)
	xp = GemRB.GetPlayerStat (pc, IE_XP)
	RecordsTextArea.Append (": "+str(xp) )
	RecordsTextArea.Append (17091,-1)
	tmp = GetNextLevelExp (levelsum, adj)
	RecordsTextArea.Append (": "+tmp )

	#current effects
	effects = GemRB.GetPlayerStates (pc)
	if len(effects):
		RecordsTextArea.Append ("\n\n[color=ffff00]")
		RecordsTextArea.Append (32052)
		RecordsTextArea.Append ("[/color][capital=2]")
		StateTable = GemRB.LoadTableObject ("statdesc")
		for c in effects:
			tmp = StateTable.GetValue (ord(c)-66, 0)
			RecordsTextArea.Append (c+" ", -1)
			RecordsTextArea.Append (tmp)

	#race
	RecordsTextArea.Append ("\n\n[capital=0][color=ffff00]")
	RecordsTextArea.Append (1048)
	RecordsTextArea.Append ("[/color]")

	RecordsTextArea.Append (Race,-1)

	#alignment
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (1049)
	RecordsTextArea.Append ("[/color]")
	tmp = AlignTable.FindValue ( 3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT) )
	Align = AlignTable.GetValue (tmp, 2)
	RecordsTextArea.Append (Align,-1)

	#saving throws
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (17379)
	RecordsTextArea.Append ("[/color]")
	tmp = GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE)
	tmp -= GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE, 1)
	if tmp<0: stmp = str(tmp)
	else: stmp = "+"+str(tmp)
	RecordsTextArea.Append (17380,-1)
	RecordsTextArea.Append (": "+stmp )
	tmp = GemRB.GetPlayerStat (pc, IE_SAVEREFLEX)
	tmp -= GemRB.GetPlayerStat (pc, IE_SAVEREFLEX, 1)
	if tmp<0: stmp = str(tmp)
	else: stmp = "+"+str(tmp)
	RecordsTextArea.Append (17381,-1)
	RecordsTextArea.Append (": "+stmp )
	tmp = GemRB.GetPlayerStat (pc, IE_SAVEWILL)
	tmp -= GemRB.GetPlayerStat (pc, IE_SAVEWILL, 1)
	if tmp<0: stmp = str(tmp)
	else: stmp = "+"+str(tmp)
	RecordsTextArea.Append (17382,-1)
	RecordsTextArea.Append (": "+stmp )

	#class features
	if HasClassFeatures(pc):
		RecordsTextArea.Append ("\n\n[color=ffff00]")
		RecordsTextArea.Append (40314)
		RecordsTextArea.Append ("[/color]\n")
		tmp = GemRB.GetPlayerStat (pc, IE_TURNUNDEADLEVEL)
		if tmp:
			RecordsTextArea.Append (12146,-1)
			RecordsTextArea.Append (": "+str(tmp) )
		tmp = GemRB.GetPlayerStat (pc, IE_BACKSTABDAMAGEMULTIPLIER)
		if tmp:
			RecordsTextArea.Append (24898,-1)
			RecordsTextArea.Append (": "+str(tmp)+"d6" )
		tmp = GemRB.GetPlayerStat (pc, IE_LAYONHANDSAMOUNT)
		if tmp:
			RecordsTextArea.Append (12127,-1)
			RecordsTextArea.Append (": "+str(tmp) )

	#bonus spells
	if HasBonusSpells(pc):
		RecordsTextArea.Append ("\n\n[color=ffff00]")
		RecordsTextArea.Append (10344)
		RecordsTextArea.Append ("[/color]\n")

	#ability statistics
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (40315)
	RecordsTextArea.Append ("[/color]")

	RecordsTextArea.Append (10338,-1) #strength
	tmp = GemRB.GetAbilityBonus( IE_STR, 3, GemRB.GetPlayerStat(pc, IE_STR) )
	RecordsTextArea.Append (": " + str(tmp) )
	tmp = GetAbilityBonus(pc, IE_CON)
	RecordsTextArea.Append (10342,-1)
	RecordsTextArea.Append (": " + str(tmp) ) #con bonus

	RecordsTextArea.Append (15581,-1) #spell resistance
	tmp = GemRB.GetPlayerStat (pc, IE_MAGICDAMAGERESISTANCE)
	RecordsTextArea.Append (": "+str(tmp) )

	return

def DisplayWeapons (pc):
	Window = RecordsWindow

	return

def DisplaySkills (pc):
	Window = RecordsWindow

	SkillTable = GemRB.LoadTableObject ("skillsta")
	SkillName = GemRB.LoadTableObject ("skills")
	rows = SkillTable.GetRowCount ()

	#skills
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (11983)
	RecordsTextArea.Append ("[/color]")

	for i in range(rows):
		stat = SkillTable.GetValue (i, 0, 2)
		value = GemRB.GetPlayerStat (pc, stat)
		base = GemRB.GetPlayerStat (pc, stat, 1)

		if value:
			skill = SkillName.GetValue (i, 1)
			RecordsTextArea.Append (skill, -1)
			RecordsTextArea.Append (" "+str(value)+"("+str(base)+")")


	FeatTable = GemRB.LoadTableObject ("featreq")
	FeatName = GemRB.LoadTableObject ("feats")
	rows = FeatTable.GetRowCount ()
	#feats
	featbits = [GemRB.GetPlayerStat (pc, IE_FEATS1), GemRB.GetPlayerStat (pc, IE_FEATS2), GemRB.GetPlayerStat (pc, IE_FEATS3)]
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (36361)
	RecordsTextArea.Append ("[/color]")

	for i in range(rows):
		featidx = i/32
		pos = 1<<(i%32)
		if featbits[featidx]&pos:
			feat = FeatName.GetValue (i, 1)
			RecordsTextArea.Append (feat, -1)
			stat = FeatTable.GetValue (i, 9, 2)
			if stat:
				multi = GemRB.GetPlayerStat (pc, stat)
				RecordsTextArea.Append (": "+str(multi) )

	return

#character information
def DisplayMisc (pc):
	Window = RecordsWindow

	TotalPartyExp = 0
	ChapterPartyExp = 0
	TotalCount = 0
	ChapterCount = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsTotalXP']
		ChapterPartyExp = ChapterPartyExp + stat['KillsChapterXP']
		TotalCount = TotalCount + stat['KillsTotalCount']
		ChapterCount = ChapterCount + stat['KillsChapterCount']

	stat = GemRB.GetPCStats (pc)

	#favourites
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (40320)
	RecordsTextArea.Append ("[/color]\n")

	#favourite spell
	RecordsTextArea.Append (stat['FavouriteSpell'])
	RecordsTextArea.Append (stat['FavouriteWeapon'])

	#
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (40322)
	RecordsTextArea.Append ("[/color]\n")

	#most powerful vanquished
	#we need getstring, so -1 will translate to empty string
	RecordsTextArea.Append (GemRB.GetString (stat['BestKilledName']))

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

	#actually it is 16043 <DURATION>, but duration is translated to
	#16041, hopefully this won't cause problem with international version
	RecordsTextArea.Append (16041)

	#total xp
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		RecordsTextArea.Append (str (PartyExp) + '%')
	else:
		RecordsTextArea.Append ("0%")

	if ChapterPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / ChapterPartyExp)
		RecordsTextArea.Append (str (PartyExp) + '%')
	else:
		RecordsTextArea.Append ("0%")

	#total xp
	if TotalCount != 0:
		PartyExp = int ((stat['KillsTotalCount'] * 100) / TotalCount)
		RecordsTextArea.Append (str (PartyExp) + '%')
	else:
		RecordsTextArea.Append ("0%")

	if ChapterCount != 0:
		PartyExp = int ((stat['KillsChapterCount'] * 100) / ChapterCount)
		RecordsTextArea.Append (str (PartyExp) + '%')
	else:
		RecordsTextArea.Append ("0%")

	RecordsTextArea.Append (str (stat['KillsChapterXP']))
	RecordsTextArea.Append (str (stat['KillsTotalXP']))

	#count of kills in chapter/game
	RecordsTextArea.Append (str (stat['KillsChapterCount']))
	RecordsTextArea.Append (str (stat['KillsTotalCount']))

	return

def RefreshRecordsWindow ():
	global RecordsTextArea

	Window = RecordsWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	#name
	Label = Window.GetControl (0x1000000e)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	#portrait
	Button = Window.GetControl (2)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))

	# armorclass
	Label = Window.GetControl (0x10000028)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))
	Label.SetTooltip (17183)

	# hp now
	Label = Window.GetControl (0x10000029)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
	Label.SetTooltip (17184)

	# hp max
	Label = Window.GetControl (0x1000002a)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
	Label.SetTooltip (17378)

 	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	dstr = sstr-GemRB.GetPlayerStat (pc, IE_STR,1)
	bstr = GetAbilityBonus (pc, IE_STR)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	dint = sint-GemRB.GetPlayerStat (pc, IE_INT,1)
	bint = GetAbilityBonus (pc, IE_INT)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	dwis = swis-GemRB.GetPlayerStat (pc, IE_WIS,1)
	bwis = GetAbilityBonus (pc, IE_WIS)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	ddex = sdex-GemRB.GetPlayerStat (pc, IE_DEX,1)
	bdex = GetAbilityBonus (pc, IE_DEX)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	dcon = scon-GemRB.GetPlayerStat (pc, IE_CON,1)
	bcon = GetAbilityBonus (pc, IE_CON)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	dchr = schr-GemRB.GetPlayerStat (pc, IE_CHR,1)
	bchr = GetAbilityBonus (pc, IE_CHR)

	Label = Window.GetControl (0x1000002f)
	Label.SetText (str(sstr))
	ColorDiff2 (Window, Label, dstr)

	Label = Window.GetControl (0x10000009)
	Label.SetText (str(sdex))
	ColorDiff2 (Window, Label, ddex)

	Label = Window.GetControl (0x1000000a)
	Label.SetText (str(scon))
	ColorDiff2 (Window, Label, dcon)

	Label = Window.GetControl (0x1000000b)
	Label.SetText (str(sint))
	ColorDiff2 (Window, Label, dint)

	Label = Window.GetControl (0x1000000c)
	Label.SetText (str(swis))
	ColorDiff2 (Window, Label, dwis)

	Label = Window.GetControl (0x1000000d)
	Label.SetText (str(schr))
	ColorDiff2 (Window, Label, dchr)

	Label = Window.GetControl (0x10000035)
	Label.SetText (str(bstr))
	ColorDiff (Window, Label, bstr)

	Label = Window.GetControl (0x10000036)
	Label.SetText (str(bdex))
	ColorDiff (Window, Label, bdex)

	Label = Window.GetControl (0x10000037)
	Label.SetText (str(bcon))
	ColorDiff (Window, Label, bcon)

	Label = Window.GetControl (0x10000038)
	Label.SetText (str(bint))
	ColorDiff (Window, Label, bint)

	Label = Window.GetControl (0x10000039)
	Label.SetText (str(bwis))
	ColorDiff (Window, Label, bwis)

	Label = Window.GetControl (0x1000003a)
	Label.SetText (str(bchr))
	ColorDiff (Window, Label, bchr)

	RecordsTextArea = Window.GetControl (45)
	RecordsTextArea.SetText ("")
	RecordsTextArea.Append ("[capital=0]")

	SelectWindow = GemRB.GetVar ("SelectWindow")
	if SelectWindow == 1:
		DisplayGeneral (pc)
	elif SelectWindow == 2:
		DisplayWeapons (pc)
	elif SelectWindow == 3:
		DisplaySkills (pc)
	elif SelectWindow == 4:
		DisplayMisc (pc)

	#if actor is uncontrollable, make this grayed
	Window.SetVisible (1)
	PortraitWindow.SetVisible (1)
	OptionsWindow.SetVisible (1)
	return

def CloseHelpWindow ():
	global DescTable

	if InformationWindow:
		InformationWindow.Unload ()
	if DescTable:
		DescTable = None
	return

#ingame help
def OpenHelpWindow ():
	global HelpTable, InformationWindow

	InformationWindow = Window = GemRB.LoadWindowObject (57)

	HelpTable = GemRB.LoadTableObject ("topics")
	GemRB.SetVar("Topic", 0)
	GemRB.SetVar("TopIndex", 0)

	for i in range(11):
		title = HelpTable.GetValue (i, 0)
		Button = Window.GetControl (i+27)
		Label = Window.GetControl (i+0x10000004)

		Button.SetVarAssoc ("Topic", i)
		if title:
			Label.SetText (title)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "UpdateHelpWindow")
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	#done
	Button = Window.GetControl (1)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseHelpWindow")
	Window.ShowModal (MODAL_SHADOW_GRAY)
	UpdateHelpWindow ()
	return

def UpdateHelpWindow ():
	global DescTable, Topic

	Window = InformationWindow

	if Topic!=GemRB.GetVar ("Topic"):
		Topic = GemRB.GetVar ("Topic")
		GemRB.SetVar ("TopIndex",0)
		GemRB.SetVar ("Selected",0)

	for i in range(11):
		Button = Window.GetControl (i+27)
		Label = Window.GetControl (i+0x10000004)
		if Topic==i:
			Label.SetTextColor (255,255,0)
		else:
			Label.SetTextColor (255,255,255)

	resource = HelpTable.GetValue (Topic, 1)
	if DescTable:
		DescTable = None

	DescTable = GemRB.LoadTableObject (resource)

	ScrollBar = Window.GetControl (4)

	startrow = HelpTable.GetValue (Topic, 4)
	if startrow<0:
		i=-startrow-10
	else:
		i = DescTable.GetRowCount ()-10-startrow

	if i<1: i=1

	ScrollBar.SetVarAssoc ("TopIndex", i)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshHelpWindow")

	RefreshHelpWindow ()
	return

def RefreshHelpWindow ():
	Window = InformationWindow
	Topic = GemRB.GetVar ("Topic")
	TopIndex = GemRB.GetVar ("TopIndex")
	Selected = GemRB.GetVar ("Selected")

	titlecol = HelpTable.GetValue (Topic, 2)
	desccol = HelpTable.GetValue (Topic, 3)
	startrow = HelpTable.GetValue (Topic, 4)
	if startrow<0: startrow = 0

	for i in range(11):
		title = DescTable.GetValue (i+startrow+TopIndex, titlecol)

		Button = Window.GetControl (i+71)
		Label = Window.GetControl (i+0x10000030)

		if i+TopIndex==Selected:
			Label.SetTextColor (255,255,0)
		else:
			Label.SetTextColor (255,255,255)
		if title>0:
			Label.SetText (title)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetVarAssoc ("Selected", i+TopIndex)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshHelpWindow")
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	if Selected<0:
		desc=""
	else:
		desc = DescTable.GetValue (Selected+startrow, desccol)

	Window = InformationWindow
	TextArea = Window.GetControl (2)
	TextArea.SetText (desc)
	return

def CloseBiographyWindow ():
	if BiographyWindow:
		BiographyWindow.Unload ()
	return

def OpenBiographyWindow ():
	global BiographyWindow

	BiographyWindow = Window = GemRB.LoadWindowObject (12)

	pc = GemRB.GameGetSelectedPCSingle ()

	TextArea = Window.GetControl (0)
	TextArea.SetText (GemRB.GetPlayerString(pc, 63) )

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseBiographyWindow")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenExportWindow ():
	global ExportWindow, NameField, ExportDoneButton

	ExportWindow = GemRB.LoadWindowObject(13)

	TextArea = ExportWindow.GetControl(2)
	TextArea.SetText(10962)

	TextArea = ExportWindow.GetControl(0)
	TextArea.GetCharacters ()

	ExportDoneButton = ExportWindow.GetControl(4)
	ExportDoneButton.SetText(11973)
	ExportDoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ExportWindow.GetControl(5)
	CancelButton.SetText(13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	NameField = ExportWindow.GetControl(6)

	ExportDoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExportDonePress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExportCancelPress")
	NameField.SetEvent(IE_GUI_EDIT_ON_CHANGE, "ExportEditChanged")
	ExportWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def ExportDonePress():
	if ExportWindow:
		ExportWindow.Unload()
	#save file under name from EditControl
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SaveCharacter(pc, ExportFileName)
	return

def ExportCancelPress():
	if ExportWindow:
		ExportWindow.Unload()
	return

def ExportEditChanged():
	global ExportFileName

	ExportFileName = NameField.QueryText()
	if ExportFileName == "":
		ExportDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		ExportDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

###################################################
# End of file GUIREC.py
