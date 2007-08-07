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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id: GUIINV.py 4503 2007-02-27 16:43:46Z wjpalenstijn $


# GUIREC.py - scripts to control character record windows from GUIREC winpack

###################################################

import string
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
		GemRB.UnloadWindow (RecordsWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

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
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenRecordsWindow")
	GemRB.SetWindowFrame (Window)

	#portrait icon
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	#information (help files)
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 11946)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenHelpWindow")

	#biography
	Button = GemRB.GetControl (Window, 59)
	GemRB.SetText (Window, Button, 18003)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	#export
	Button = GemRB.GetControl (Window, 36)
	GemRB.SetText (Window, Button, 13956)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenExportWindow")

	#customize
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetText (Window, Button, 10645)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenCustomizeWindow")

	#general
	GemRB.SetVar ("SelectWindow", 1)

	Button = GemRB.GetControl (Window, 60)
	GemRB.SetTooltip (Window, Button, 40316)
	GemRB.SetVarAssoc (Window, Button, "SelectWindow", 1)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#weapons and armour
	Button = GemRB.GetControl (Window, 61)
	GemRB.SetTooltip (Window, Button, 40317)
	GemRB.SetVarAssoc (Window, Button, "SelectWindow", 2)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#skills and feats
	Button = GemRB.GetControl (Window, 62)
	GemRB.SetTooltip (Window, Button, 40318)
	GemRB.SetVarAssoc (Window, Button, "SelectWindow", 3)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#miscellaneous
	Button = GemRB.GetControl (Window, 63)
	GemRB.SetTooltip (Window, Button, 33500)
	GemRB.SetVarAssoc (Window, Button, "SelectWindow", 4)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshRecordsWindow")

	#level up
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetText (Window, Button, 7175)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LevelUpWindow")

	SetSelectionChangeHandler (RefreshRecordsWindow)

	RefreshRecordsWindow ()
	return

def ColorDiff (Window, Label, diff):
#this is the bg2 style color diff
#	if diff>0:
#		GemRB.SetLabelTextColor (Window, Label, 0, 255, 0)
#	elif diff<0:
#		GemRB.SetLabelTextColor (Window, Label, 255, 0, 0)
#	else:
#		GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)

#this is the iwd2 style color diff
	if diff:
		GemRB.SetLabelTextColor (Window, Label, 255, 255, 0)
	else:
		GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)
	return

def DisplayGeneral (pc):
	Window = RecordsWindow
	RaceTable = GemRB.LoadTable("races")
	ClassTable = GemRB.LoadTable("classes")
	AlignTable = GemRB.LoadTable("aligns")

	#levels
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 40308)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")

	#experience
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17089)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")

	GemRB.TextAreaAppend (Window, RecordsTextArea, 36928)
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17091)

	#current effects
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 32052)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]")
	effects = GemRB.GetPlayerStates (pc)
	StateTable = GemRB.LoadTable ("statdesc") 
	for c in effects:
		tmp = GemRB.GetTableValue (StateTable, ord(c)-65, 0)
		print c, GemRB.GetString(tmp)
		GemRB.TextAreaAppend (Window, RecordsTextArea, tmp, -1)
 	GemRB.UnloadTable (StateTable)

	#race
	GemRB.TextAreaAppend (Window, RecordsTextArea, "\n\n[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 1048)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")
	Race = GemRB.GetTableValue(RaceTable,GemRB.GetPlayerStat(pc,IE_RACE)-1,2)
	GemRB.TextAreaAppend (Window, RecordsTextArea, Race)

	#alignment
	GemRB.TextAreaAppend (Window, RecordsTextArea, "\n\n[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 1049)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")
	tmp = GemRB.FindTableValue ( AlignTable, 3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT) )
	Align = GemRB.GetTableValue (AlignTable, tmp, 0)
	GemRB.TextAreaAppend (Window, RecordsTextArea, Align)

	#saving throws
	GemRB.TextAreaAppend (Window, RecordsTextArea, "\n\n[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17379)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")
	tmp = GemRB.FindTableValue ( AlignTable, 3, GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE) )
	Save = GemRB.GetTableValue (AlignTable, tmp, 0)
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17380)
	GemRB.TextAreaAppend (Window, RecordsTextArea, Save)
	tmp = GemRB.FindTableValue ( AlignTable, 3, GemRB.GetPlayerStat (pc, IE_SAVEREFLEX) )
	Align = GemRB.GetTableValue (AlignTable, tmp, 0)
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17381)
	GemRB.TextAreaAppend (Window, RecordsTextArea, Save)
	tmp = GemRB.FindTableValue ( AlignTable, 3, GemRB.GetPlayerStat (pc, IE_SAVEWILL) )
	Align = GemRB.GetTableValue (AlignTable, tmp, 0)
	GemRB.TextAreaAppend (Window, RecordsTextArea, 17382)
	GemRB.TextAreaAppend (Window, RecordsTextArea, Save)

	GemRB.UnloadTable (RaceTable)
	GemRB.UnloadTable (ClassTable)
	GemRB.UnloadTable (AlignTable)
	return

def DisplayWeapons (pc):
	Window = RecordsWindow

	return

def DisplaySkills (pc):
	Window = RecordsWindow

	SkillTable = GemRB.LoadTable ("skillsta")
	SkillName = GemRB.LoadTable ("skills")
	rows = GemRB.GetTableRowCount (SkillTable)

	#skills
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 11983)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]")

	for i in range(rows):
		stat = GemRB.GetTableValue (SkillTable, i, 0, 2)
		value = GemRB.GetPlayerStat (pc, stat)
		base = GemRB.GetPlayerStat (pc, stat, 1)

		if value:			
			skill = GemRB.GetTableValue (SkillName, i, 1)
			GemRB.TextAreaAppend (Window, RecordsTextArea, skill, -1)
			GemRB.TextAreaAppend (Window, RecordsTextArea, " "+str(value)+"("+str(base)+")")

	GemRB.UnloadTable (SkillTable)
	GemRB.UnloadTable (SkillName)

	FeatTable = GemRB.LoadTable ("featreq")
	FeatName = GemRB.LoadTable ("feats")
	rows = GemRB.GetTableRowCount (FeatTable)
	#feats
	featbits = [GemRB.GetPlayerStat (pc, IE_FEATS1), GemRB.GetPlayerStat (pc, IE_FEATS2), GemRB.GetPlayerStat (pc, IE_FEATS3)]
	GemRB.TextAreaAppend (Window, RecordsTextArea, "\n\n[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 36361)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]")

	for i in range(rows):		
		featidx = i/32
		pos = 1<<(i%32)
		if featbits[featidx]&pos:
			feat = GemRB.GetTableValue (FeatName, i, 1)
			GemRB.TextAreaAppend (Window, RecordsTextArea, feat, -1)
			stat = GemRB.GetTableValue (FeatTable, i, 9, 2)
			print stat
			if stat:
				multi = GemRB.GetPlayerStat (pc, stat)
				GemRB.TextAreaAppend (Window, RecordsTextArea, ": "+str(multi) )

	GemRB.UnloadTable (FeatTable)
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
		TotalPartyExp = TotalPartyExp + stat['KillsChapterXP']
		ChapterPartyExp = ChapterPartyExp + stat['KillsTotalXP']
		TotalCount = TotalCount + stat['KillsChapterCount']
		ChapterCount = ChapterCount + stat['KillsTotalCount']

	stat = GemRB.GetPCStats (pc)

	#favourites
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 40320)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")

	#favourite spell
	GemRB.TextAreaAppend (Window, RecordsTextArea, stat['FavouriteSpell'])
	GemRB.TextAreaAppend (Window, RecordsTextArea, stat['FavouriteWeapon'])

	#
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[color=0xffff00]")
	GemRB.TextAreaAppend (Window, RecordsTextArea, 40322)
	GemRB.TextAreaAppend (Window, RecordsTextArea, "[/color]\n")

	#most powerful vanquished
	#we need getstring, so -1 will translate to empty string
	GemRB.TextAreaAppend (Window, RecordsTextArea, GemRB.GetString (stat['BestKilledName']))

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
	GemRB.TextAreaAppend (Window, RecordsTextArea, 16041)

	#total xp 
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		GemRB.TextAreaAppend (Window, RecordsTextArea, str (PartyExp) + '%')
	else:
		GemRB.TextAreaAppend (Window, RecordsTextArea, "0%")

	if ChapterPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / ChapterPartyExp)
		GemRB.TextAreaAppend (Window, RecordsTextArea, str (PartyExp) + '%')
	else:
		GemRB.TextAreaAppend (Window, RecordsTextArea, "0%")

	#total xp 
	if TotalCount != 0:
		PartyExp = int ((stat['KillsTotalCount'] * 100) / TotalCount)
		GemRB.TextAreaAppend (Window, RecordsTextArea, str (PartyExp) + '%')
	else:
		GemRB.TextAreaAppend (Window, RecordsTextArea, "0%")

	if ChapterCount != 0:
		PartyExp = int ((stat['KillsChapterCount'] * 100) / ChapterCount)
		GemRB.TextAreaAppend (Window, RecordsTextArea, str (PartyExp) + '%')
	else:
		GemRB.TextAreaAppend (Window, RecordsTextArea, "0%")

	GemRB.TextAreaAppend (Window, RecordsTextArea, str (stat['KillsChapterXP']))
	GemRB.TextAreaAppend (Window, RecordsTextArea, str (stat['KillsTotalXP']))

	#count of kills in chapter/game
	GemRB.TextAreaAppend (Window, RecordsTextArea, str (stat['KillsChapterCount']))
	GemRB.TextAreaAppend (Window, RecordsTextArea, str (stat['KillsTotalCount']))

	return 

def RefreshRecordsWindow ():
	global RecordsTextArea

	Window = RecordsWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	#name
	Label = GemRB.GetControl (Window, 0x1000000e)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	#portrait 
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonPicture (Window, Button, GemRB.GetPlayerPortrait (pc,0)) 

	# armorclass
	Label = GemRB.GetControl (Window, 0x10000028)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))
	GemRB.SetTooltip (Window, Label, 17183)

	# hp now
	Label = GemRB.GetControl (Window, 0x10000029)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
	GemRB.SetTooltip (Window, Label, 17184)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000002a)
	GemRB.SetText (Window, Label, str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
	GemRB.SetTooltip (Window, Label, 17378)
 
 	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	dstr = sstr-GemRB.GetPlayerStat (pc, IE_STR,1)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	dint = sint-GemRB.GetPlayerStat (pc, IE_INT,1)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	dwis = swis-GemRB.GetPlayerStat (pc, IE_WIS,1)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	ddex = sdex-GemRB.GetPlayerStat (pc, IE_DEX,1)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	dcon = scon-GemRB.GetPlayerStat (pc, IE_CON,1)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	dchr = schr-GemRB.GetPlayerStat (pc, IE_CHR,1)

	Label = GemRB.GetControl (Window, 0x1000002f)
	GemRB.SetText (Window, Label, str(sstr))
	ColorDiff (Window, Label, dstr)

	Label = GemRB.GetControl (Window, 0x10000009)
	GemRB.SetText (Window, Label, str(sdex))
	ColorDiff (Window, Label, ddex)

	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, str(scon))
	ColorDiff (Window, Label, dcon)

	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str(sint))
	ColorDiff (Window, Label, dint)

	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, str(swis))
	ColorDiff (Window, Label, dwis)

	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, str(schr))
	ColorDiff (Window, Label, dchr)

	RecordsTextArea = GemRB.GetControl (Window, 45)
	GemRB.SetText (	Window, RecordsTextArea, "")

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
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	GemRB.SetVisible (OptionsWindow, 1)
	return

def CloseHelpWindow ():
	global DescTable

	GemRB.UnloadWindow (InformationWindow)
	GemRB.UnloadTable (HelpTable)
	if DescTable:
		GemRB.UnloadTable (DescTable)
		DescTable = None
	return

#ingame help 
def OpenHelpWindow ():
	global HelpTable, InformationWindow

	InformationWindow = Window = GemRB.LoadWindow (57)

	HelpTable = GemRB.LoadTable ("topics")
	GemRB.SetVar("Topic", 0)
	GemRB.SetVar("TopIndex", 0)

	for i in range(11):
		title = GemRB.GetTableValue (HelpTable, i, 0)
		Button = GemRB.GetControl (Window, i+27)
		Label = GemRB.GetControl (Window, i+0x10000004)

		GemRB.SetVarAssoc (Window, Button, "Topic", i)
		if title:
			GemRB.SetText (Window, Label, title)			
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateHelpWindow")
		else:
			GemRB.SetText (Window, Label, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	#done
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseHelpWindow")
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
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
		Button = GemRB.GetControl (Window, i+27)
		Label = GemRB.GetControl (Window, i+0x10000004)
		if Topic==i:
			GemRB.SetLabelTextColor (Window, Label, 255,255,0)
		else:
			GemRB.SetLabelTextColor (Window, Label, 255,255,255)
		
	resource = GemRB.GetTableValue (HelpTable, Topic, 1)
	if DescTable:
		GemRB.UnloadTable (DescTable)
		DescTable = None

	DescTable = GemRB.LoadTable (resource)

	ScrollBar = GemRB.GetControl (Window, 4)

	startrow = GemRB.GetTableValue (HelpTable, Topic, 4)
	if startrow<0:
		i=-startrow-10
	else:
		i = GemRB.GetTableRowCount (DescTable)-10-startrow

	if i<1: i=1

	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", i)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshHelpWindow") 

	RefreshHelpWindow ()
	return

def RefreshHelpWindow ():
	Window = InformationWindow
	Topic = GemRB.GetVar ("Topic")
	TopIndex = GemRB.GetVar ("TopIndex")
	Selected = GemRB.GetVar ("Selected")

	titlecol = GemRB.GetTableValue (HelpTable, Topic, 2)
	desccol = GemRB.GetTableValue (HelpTable, Topic, 3)
	startrow = GemRB.GetTableValue (HelpTable, Topic, 4)
	if startrow<0: startrow = 0
		
	for i in range(11):
		title = GemRB.GetTableValue (DescTable, i+startrow+TopIndex, titlecol)
		
		Button = GemRB.GetControl (Window, i+71)
		Label = GemRB.GetControl (Window, i+0x10000030)
		
		if i+TopIndex==Selected:
			GemRB.SetLabelTextColor (Window, Label, 255,255,0)
		else:
			GemRB.SetLabelTextColor (Window, Label, 255,255,255)
		if title>0:
			GemRB.SetText (Window, Label, title)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
			GemRB.SetVarAssoc (Window, Button, "Selected", i+TopIndex)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshHelpWindow")
		else:
			GemRB.SetText (Window, Label, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	if Selected<0:
		desc=""
	else:
		desc = GemRB.GetTableValue (DescTable, Selected+startrow, desccol)

	Window = InformationWindow
	TextArea = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, TextArea, desc)
	return

def CloseBiographyWindow ():
	GemRB.UnloadWindow (BiographyWindow)
	return

def OpenBiographyWindow ():
	global BiographyWindow

	BiographyWindow = Window = GemRB.LoadWindow (12)

	pc = GemRB.GameGetSelectedPCSingle ()

	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, GemRB.GetPlayerString(pc, 63) )

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseBiographyWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return 

###################################################
# End of file GUIREC.py
