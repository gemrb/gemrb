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


# GUIREC.py - scripts to control character record windows from GUIREC winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
import GUIRECCommon
from GUIDefines import *
from ie_stats import *
from ie_restype import *
from ie_feats import FEAT_WEAPON_FINESSE

SelectWindow = 0
Topic = None
HelpTable = None
DescTable = None
RecordsWindow = None
RecordsTextArea = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
InformationWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow, SelectWindow

	if GUICommon.CloseOtherWindow (OpenRecordsWindow):
		if RecordsWindow:
			RecordsWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIREC", 800, 600)
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenRecordsWindow)
	Window.SetFrame ()

	#portrait icon
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	#information (help files)
	Button = Window.GetControl (1)
	Button.SetText (11946)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenHelpWindow)

	#biography
	Button = Window.GetControl (59)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenBiographyWindow)

	#export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenExportWindow)

	#customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenCustomizeWindow)

	#general
	GemRB.SetVar ("SelectWindow", 1)

	Button = Window.GetControl (60)
	Button.SetTooltip (40316)
	Button.SetVarAssoc ("SelectWindow", 1)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#weapons and armour
	Button = Window.GetControl (61)
	Button.SetTooltip (40317)
	Button.SetVarAssoc ("SelectWindow", 2)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#skills and feats
	Button = Window.GetControl (62)
	Button.SetTooltip (40318)
	Button.SetVarAssoc ("SelectWindow", 3)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#miscellaneous
	Button = Window.GetControl (63)
	Button.SetTooltip (33500)
	Button.SetVarAssoc ("SelectWindow", 4)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#level up
	Button = Window.GetControl (37)
	Button.SetText (7175)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow) #TODO: OpenLevelUpWindow

	GUICommonWindows.SetSelectionChangeHandler (UpdateRecordsWindow)

	UpdateRecordsWindow ()
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
	if Adjustment>5:
		Adjustment = 5
	if (Level < CommonTables.NextLevel.GetColumnCount (4) - 5):
		return str(CommonTables.NextLevel.GetValue (4, Level + Adjustment ) )

	return GemRB.GetString(24342) #godhood

#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = [IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
IE_LEVELSORCEROR, IE_LEVEL2]

# screenshots at http:// lparchive.org/Icewind-Dale-2/Update%2013/
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
			Class = GUICommonWindows.GetActorClassTitle (pc, i )
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
	AlignTable = GemRB.LoadTable("aligns")

	#get the subrace value
	Value = GemRB.GetPlayerStat(pc,IE_RACE)
	Value2 = GemRB.GetPlayerStat(pc,IE_SUBRACE)
	if Value2:
		Value = Value<<16 | Value2
	tmp = CommonTables.Races.FindValue (3, Value)
	Race = CommonTables.Races.GetValue (tmp, 2)
	tmp = CommonTables.Races.GetValue (tmp, 8)

	Label = Window.GetControl (0x1000000f)
	Label.SetText (Race)

	if tmp == -1:
		tmp = highest
	else:
		tmp = GetFavoredClass(pc, tmp)

	tmp = CommonTables.Classes.GetValue (tmp, 0)
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
		RecordsTextArea.Append ("[/color]")
		StateTable = GemRB.LoadTable ("statdesc")
		for c in effects:
			tmp = StateTable.GetValue (str(ord(c)-66), "DESCRIPTION")
			RecordsTextArea.Append ("[capital=2]"+c+" ", -1)
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

#FIXME: display only nonzero values except for casting failure
#TODO: +/- prefix
#TODO: check if there are any other entries
# screenshots at http:// lparchive.org/Icewind-Dale-2/Update%206/
def DisplayWeapons (pc):
	Window = RecordsWindow

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GB = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s, 1)
	# maybe add an iwd2 mode to GemRB.GetAbilityBonus
	GA = lambda s, pc=pc: int((GS(s)-10)/2)

	###################
	# Attack Roll Modifiers
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (9457)
	RecordsTextArea.Append ("[/color]\n")

	combatdet = GemRB.GetCombatDetails(pc, 0)
	numOfAttacks = GS (IE_NUMBEROFATTACKS)//2

	# Main Hand
	# display all the (successive) attack values (+15/+10/+5)
	total = combatdet["ToHit"]
	tmpstr = str(total)
	for i in range(1, numOfAttacks):
		tmpstr = tmpstr  + "/" + str(total-i*5)
	RecordsTextArea.Append (delimited_txt (734, ":", tmpstr, 0))
	# Base
	RecordsTextArea.Append ("  ", -1) # indentation
	RecordsTextArea.Append (delimited_txt (31353, ":", str (GS(IE_TOHIT)), 0))
	total = total - GS(IE_TOHIT)
	# Weapon bonus
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (32560 , ":", str (0), 0))
	# Proficiency bonus
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (32561, ":", str (0), 0))
	# Armor Penalty
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (39816 , ":", str (0), 0))
	# Shield Penalty (if you don't have the shield proficiency feat; maybe more)
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (39822 , ":", str (0), 0))
	# Ability  bonus
	#FIXME: this is different for ranged weapons (dex only)
	strbon = GA(IE_STR)
	dexbon = GA(IE_DEX)
	# weapon finesse uses the best of both
	if GemRB.HasFeat(pc, FEAT_WEAPON_FINESSE):
		if dexbon > strbon:
			strbon = dexbon
	if strbon:
		RecordsTextArea.Append ("  ", -1)
		RecordsTextArea.Append (delimited_txt (33547, ":", str (strbon), 0))
	total = total - strbon
	# Others
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (33548, ":", str (total))) # just the remnants of "total"
	RecordsTextArea.Append ("\n")

	# Off Hand
	if (GemRB.IsDualWielding(pc)):
		RecordsTextArea.Append (delimited_txt (733, ":", str (GemRB.GetCombatDetails(pc, 1)["ToHit"]), 0))
		RecordsTextArea.Append ("\n")
		#TODO: probably all the same categories as above


	###################
	# Number of Attacks
	RecordsTextArea.Append (delimited_txt (9458, ":", str (numOfAttacks)))
	RecordsTextArea.Append ("\n")

	###################
	# Armor Class
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (33553)
	RecordsTextArea.Append ("[/color]\n")
	RecordsTextArea.Append (delimited_txt (33553, ":", str (GS(IE_ARMORCLASS)), 0))

	# Base
	RecordsTextArea.Append ("  ", -1) # indentation
	RecordsTextArea.Append (delimited_txt (31353, ":", str (10), 0))
	# Armor
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (11997, ":", str (0), 0))
	# Shield
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (6347, ":", str (0), 0))
	# Deflection
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (33551, ":", str (0), 0))
	# Generic
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (33552, ":", str (0), 0))
	# Dexterity
	if GA(IE_DEX):
		RecordsTextArea.Append ("  ", -1)
		RecordsTextArea.Append (delimited_txt (1151, ":", str (GA(IE_DEX)), 0))
	# Monk Wisdom Bonus: <number> to AC
	if GS(IE_LEVELMONK):
		RecordsTextArea.Append ("  ", -1)
		GemRB.SetToken ("number", str (GA(IE_WIS)))
		RecordsTextArea.Append (39431, -1)
	#TODO: Dodge?

	RecordsTextArea.Append ("\n\n")

	###################
	# Armor Class Modifiers
	stat = GS (IE_ACMISSILEMOD) + GS (IE_ACSLASHINGMOD) + GS (IE_ACPIERCINGMOD) + GS (IE_ACCRUSHINGMOD)
	if stat:
		RecordsTextArea.Append ("[color=ffff00]")
		RecordsTextArea.Append (11766)
		RecordsTextArea.Append ("[/color]")

		# Missile
		if GS (IE_ACMISSILEMOD):
			RecordsTextArea.Append ("  ", -1) # indentation
			RecordsTextArea.Append (delimited_txt (11767, ":", str (GS (IE_ACMISSILEMOD)), 0))
		# Slashing
		if GS (IE_ACSLASHINGMOD):
			RecordsTextArea.Append ("  ", -1)
			RecordsTextArea.Append (delimited_txt (11768, ":", str (GS (IE_ACSLASHINGMOD)), 0))
		# Piercing
		if GS (IE_ACPIERCINGMOD):
			RecordsTextArea.Append ("  ", -1)
			RecordsTextArea.Append (delimited_txt (11769, ":", str (GS (IE_ACPIERCINGMOD)), 0))
		# Bludgeoning
		if GS (IE_ACCRUSHINGMOD):
			RecordsTextArea.Append ("  ", -1)
			RecordsTextArea.Append (delimited_txt (11770, ":", str (GS (IE_ACCRUSHINGMOD))))

		RecordsTextArea.Append ("\n")

	###################
	# Arcane spell failure
	if GS(IE_LEVELBARD) + GS(IE_LEVELSORCEROR) + GS(IE_LEVELMAGE):
		RecordsTextArea.Append ("[color=ffff00]")
		RecordsTextArea.Append (41391)
		RecordsTextArea.Append ("[/color]\n")

		# Casting Failure
		RecordsTextArea.Append (delimited_txt (41390 , ":", str (GS(IE_SPELLFAILUREMAGE)), 0))
		# Armor Penalty
		RecordsTextArea.Append ("  ", -1)
		RecordsTextArea.Append (delimited_txt (39816 , ":", str (0), 0))
		# Shield Penalty
		RecordsTextArea.Append ("  ", -1)
		RecordsTextArea.Append (delimited_txt (39822, ":", str (0), 0))
		#TODO: check if there's also the bonus from armored arcana

		RecordsTextArea.Append ("\n\n")

	###################
	# Weapon Statistics
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (41119)
	RecordsTextArea.Append ("[/color]\n")

	slot_item = GemRB.GetSlotItem (pc, GemRB.GetEquippedQuickSlot (pc) )
	if not slot_item:
		print "ARGHH, no slot item, bailing out"
		return
	item = GemRB.GetItem (slot_item["ItemResRef"])
	##FIXME: display Ranged (41123) + ammo for ranged weapons
	RecordsTextArea.Append (delimited_str (734, " -", item["ItemNameIdentified"], 0))

	# Damage
	# TODO: display the unresolved damage string (2d6)
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (39518, ":", str (0), 0))
	# Strength
	# TODO: check if the weapon takes strength bonus at all
	if GA(IE_STR):
		RecordsTextArea.Append ("  ", -1)
		RecordsTextArea.Append (delimited_txt (1145, ":", str (GA(IE_STR)), 0))
	# Launcher
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (41408, ":", str (0), 0))
	# Damage Potential
	# TODO: display the unresolved total damage potential (2-12)
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (41120, ":", str (0), 0))
	# Critical Hit (19-20 / x2)
	# TODO: display the number of rolls and check if the critical range is already ok
	crange = 20 - combatdet["CriticalBonus"]
	if crange == 20:
		crange = "20 / x" + str(1)
	else:
		crange = str(crange) + "-20 / x" + str(1)
	RecordsTextArea.Append ("  ", -1)
	RecordsTextArea.Append (delimited_txt (41122, ":", crange, 0))

	#TODO: probably repeat for the off-hand

	return

def DisplaySkills (pc):
	Window = RecordsWindow

	SkillTable = GemRB.LoadTable ("skillsta")
	SkillName = GemRB.LoadTable ("skills")
	rows = SkillTable.GetRowCount ()

	#skills
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (11983)
	RecordsTextArea.Append ("[/color]\n")

	skills = []
	for i in range(rows):
		stat = SkillTable.GetValue (i, 0, 2)
		value = GemRB.GetPlayerStat (pc, stat)
		base = GemRB.GetPlayerStat (pc, stat, 1)

		if value:
			skill = SkillName.GetValue (i, 1)
			skills.append (GemRB.GetString(skill) + ": " + str(value) + " (" + str(base) + ")\n")

	skills.sort()
	for i in skills:
		RecordsTextArea.Append (i)

	FeatTable = GemRB.LoadTable ("featreq")
	FeatName = GemRB.LoadTable ("feats")
	rows = FeatTable.GetRowCount ()
	#feats
	RecordsTextArea.Append ("\n\n[color=ffff00]")
	RecordsTextArea.Append (36361)
	RecordsTextArea.Append ("[/color]\n")

	feats = []
	for i in range(rows):
		if GemRB.HasFeat(pc, i):
			feat = FeatName.GetValue (i, 1)
			stat = FeatTable.GetValue (i, 0, 2)
			if stat:
				multi = GemRB.GetPlayerStat (pc, stat)
				feats.append (GemRB.GetString(feat) + ": " + str(multi) + "\n")
			else:
				feats.append (GemRB.GetString(feat) + "\n")

	feats.sort()
	for i in feats:
		RecordsTextArea.Append (i)

	return

def delimited_str(strref, delimiter, strref2, newline=1):
	if strref2:
		val = GemRB.GetString(strref) + delimiter + " " + GemRB.GetString(strref2)
	else:
		val = GemRB.GetString(strref) + delimiter
	if newline:
		return val + "\n"
	else:
		return val

def delimited_txt(strref, delimiter, text, newline=1):
	val = GemRB.GetString(strref) + delimiter + " " + str(text)
	if newline:
		return val + "\n"
	else:
		return val

#character information
def DisplayMisc (pc):
	Window = RecordsWindow

	TotalPartyExp = 0
	TotalCount = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsTotalXP']
		TotalCount = TotalCount + stat['KillsTotalCount']

	stat = GemRB.GetPCStats (pc)

	#favourites
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (40320)
	RecordsTextArea.Append ("[/color]\n")

	#favourite spell and weapon
	RecordsTextArea.Append (delimited_str (11949, ":", stat['FavouriteSpell']))
	RecordsTextArea.Append (delimited_str (11950, ":", stat['FavouriteWeapon']))

	# combat details
	RecordsTextArea.Append ("\n[color=ffff00]")
	RecordsTextArea.Append (40322)
	RecordsTextArea.Append ("[/color]\n")

	#most powerful vanquished, time spent, xp and kills
	RecordsTextArea.Append (delimited_str (11947, ":", stat['BestKilledName']))

	days, hours = GUICommon.SetCurrentDateTokens (stat)
	# iwd2 is special here
	# construct <GAMEDAYS> days ~and~ ~<HOUR> hours~
	if days == 1:
		time = GemRB.GetString (10698)
	else:
		time = GemRB.GetString (10697)
	time += " " + GemRB.GetString (10699) + " "
	if hours == 1:
		time += GemRB.GetString (10701)
	else:
		time += GemRB.GetString (10700)

	RecordsTextArea.Append (delimited_txt (11948, ":", time))

	# Experience Value of Kills
	RecordsTextArea.Append (delimited_txt (11953, ":", stat['KillsTotalXP']))

	# Number of Kills
	RecordsTextArea.Append (delimited_txt (11954, ":", stat['KillsTotalCount']))

	# Total Experience Value in Party
	if TotalPartyExp:
		val = stat['KillsTotalXP']*100/TotalPartyExp
	else:
		val = 0
	RecordsTextArea.Append (delimited_txt (11951, ":", str(val) + "%"))

	# Percentage of Total Kills in Party
	if TotalPartyExp:
		val = stat['KillsTotalCount']*100/TotalCount
	else:
		val = 0
	RecordsTextArea.Append (delimited_txt (11954, ":", str(val) + "%"))

	return

def UpdateRecordsWindow ():
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
	Window.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseHelpWindow ():
	global DescTable, InformationWindow

	if InformationWindow:
		InformationWindow.Unload ()
		InformationWindow = None
	if DescTable:
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
		title = HelpTable.GetValue (i, 0)
		Button = Window.GetControl (i+27)
		Label = Window.GetControl (i+0x10000004)

		Button.SetVarAssoc ("Topic", i)
		if title:
			Label.SetText (title)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateHelpWindow)
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	#done
	Button = Window.GetControl (1)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseHelpWindow)
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

	DescTable = GemRB.LoadTable (resource)

	ScrollBar = Window.GetControl (4)

	startrow = HelpTable.GetValue (Topic, 4)
	if startrow<0:
		i=-startrow-10
	else:
		i = DescTable.GetRowCount ()-10-startrow

	if i<1: i=1

	ScrollBar.SetVarAssoc ("TopIndex", i)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RefreshHelpWindow)

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
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RefreshHelpWindow)
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	if Selected<0:
		desc=""
	else:
		desc = DescTable.GetValue (Selected+startrow, desccol)

	TextArea = Window.GetControl (2)
	TextArea.SetText (desc)
	return

###################################################
# End of file GUIREC.py
