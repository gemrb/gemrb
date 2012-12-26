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
from ie_feats import FEAT_WEAPON_FINESSE, FEAT_ARMORED_ARCANA

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

def DisplayCommon (pc):
	Window = RecordsWindow

	Value = GemRB.GetPlayerStat(pc,IE_RACE)
	Value2 = GemRB.GetPlayerStat(pc,IE_SUBRACE)
	if Value2:
		Value = Value<<16 | Value2
	tmp = CommonTables.Races.FindValue (3, Value)
	Race = CommonTables.Races.GetValue (tmp, 2)
	Label = Window.GetControl (0x1000000f)
	Label.SetText (Race)
	return

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

	#get the subrace value
	Value = GemRB.GetPlayerStat(pc,IE_RACE)
	Value2 = GemRB.GetPlayerStat(pc,IE_SUBRACE)
	if Value2:
		Value = Value<<16 | Value2
	tmp = CommonTables.Races.FindValue (3, Value)
	Race = CommonTables.Races.GetValue (tmp, 2)
	tmp = CommonTables.Races.GetValue (tmp, 8)

	if tmp == -1:
		tmp = highest
	else:
		tmp = GetFavoredClass(pc, tmp)

	tmp = CommonTables.Classes.GetValue (CommonTables.Classes.GetRowName(tmp), "NAME_REF")
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
	tmp = CommonTables.Aligns.FindValue (3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT))
	Align = CommonTables.Aligns.GetValue (tmp, 2)
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
		RecordsTextArea.Append ("[/color]")
		tmp = GemRB.GetPlayerStat (pc, IE_TURNUNDEADLEVEL)
		if tmp:
			RecordsTextArea.Append (12126,-1)
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
	tmp = GUICommon.GetAbilityBonus(pc, IE_CON)
	RecordsTextArea.Append (10342,-1)
	RecordsTextArea.Append (": " + str(tmp) ) #con bonus

	RecordsTextArea.Append (15581,-1) #spell resistance
	tmp = GemRB.GetPlayerStat (pc, IE_MAGICDAMAGERESISTANCE)
	RecordsTextArea.Append (": "+str(tmp) )

	return

# some of the displayed stats are manually indented
def AddIndent():
	RecordsTextArea.Append ("   ", -1)

def PlusMinusStat(value):
	if value > 0:
		return "+" + str(value)
	return str(value)

def CascadeToHit(total, ac, apr):
	cascade = PlusMinusStat(str(total))
	babDec = 5
	if ac["Wisdom"]: #TODO: also chech that there are no weapons equipped
		babDec = 3
	for i in range(1, apr):
		if total-i*babDec > 0: # skips negative ones, meaning a lower number of attacks can be displayed
			cascade = cascade  + "/" + PlusMinusStat(total-i*babDec)
	return cascade

def ToHitOfHand(combatdet, dualwielding, left=0):
	ac = combatdet["AC"]
	tohit = combatdet["ToHitStats"]

	# display all the (successive) attack values (+15/+10/+5)
	if left:
		apr = 1 # offhand gives just one extra attack
		hits = CascadeToHit(tohit["Total"], ac, apr)
		RecordsTextArea.Append (delimited_txt (733, ":", hits, 0))
	else:
		# account for the fact that the total apr contains the one for the other hand too
		if dualwielding:
			apr = combatdet["APR"]//2 - 1
		else:
			apr = combatdet["APR"]//2
		hits = CascadeToHit(tohit["Total"], ac, apr)
		RecordsTextArea.Append (delimited_txt (734, ":", hits, 0))

	# Base
	AddIndent()
	hits = CascadeToHit(tohit["Base"], ac, apr)
	RecordsTextArea.Append (delimited_txt (31353, ":", hits, 0))
	# Weapon bonus
	if tohit["Weapon"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32560 , ":", PlusMinusStat(tohit["Weapon"]), 0))
	# Proficiency bonus
	if tohit["Proficiency"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32561, ":", PlusMinusStat(tohit["Proficiency"]), 0))
	# Armor Penalty
	if tohit["Armor"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (39816 , ":", PlusMinusStat(tohit["Armor"]), 0))
	# Shield Penalty (if you don't have the shield proficiency feat)
	if tohit["Shield"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (39822 , ":", PlusMinusStat(tohit["Shield"]), 0))
	# Ability  bonus
	if tohit["Ability"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33547, ":", PlusMinusStat(tohit["Ability"]), 0))
	# Others
	if tohit["Generic"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33548, ":", PlusMinusStat(tohit["Generic"]), 0))
	RecordsTextArea.Append ("\n\n")


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
	ac = combatdet["AC"]
	tohit = combatdet["ToHitStats"]
	dualwielding = GemRB.IsDualWielding(pc)

	# Main Hand
	ToHitOfHand (combatdet, dualwielding)

	# Off Hand
	if dualwielding:
		ToHitOfHand (GemRB.GetCombatDetails(pc, 1), dualwielding, 1)


	###################
	# Number of Attacks
	if dualwielding:
		# only one extra offhand attack and it is displayed as eg. 2+1
		RecordsTextArea.Append (delimited_txt (9458, ":", str (combatdet["APR"]//2-1)+"+1"))
	else:
		RecordsTextArea.Append (delimited_txt (9458, ":", str (combatdet["APR"]//2)))
	RecordsTextArea.Append ("\n")

	###################
	# Armor Class
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (33553)
	RecordsTextArea.Append ("[/color]\n")
	RecordsTextArea.Append (delimited_txt (33553, ":", str (GS(IE_ARMORCLASS)), 0)) # same as ac["Total"]

	# Base
	AddIndent()
	RecordsTextArea.Append (delimited_txt (31353, ":", str (ac["Natural"]), 0))
	# Armor
	if ac["Armor"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (11997, ":", PlusMinusStat (ac["Armor"]), 0))
	# Shield
	if ac["Shield"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (6347, ":", PlusMinusStat (ac["Shield"]), 0))
	# Deflection
	if ac["Deflection"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33551, ":", PlusMinusStat (ac["Deflection"]), 0))
	# Generic
	if ac["Generic"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33552, ":", PlusMinusStat (ac["Generic"]), 0))
	# Dexterity
	if ac["Dexterity"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (1151, ":", PlusMinusStat (ac["Dexterity"]), 0))
	# Monk Wisdom Bonus: <number> to AC
	if ac["Wisdom"]:
		GemRB.SetToken ("number", PlusMinusStat (ac["Wisdom"]))
		RecordsTextArea.Append (39431, -1)

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
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11767, ":", PlusMinusStat(GS (IE_ACMISSILEMOD)), 0))
		# Slashing
		if GS (IE_ACSLASHINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11768, ":", PlusMinusStat (GS (IE_ACSLASHINGMOD)), 0))
		# Piercing
		if GS (IE_ACPIERCINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11769, ":", PlusMinusStat (GS (IE_ACPIERCINGMOD)), 0))
		# Bludgeoning
		if GS (IE_ACCRUSHINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11770, ":", PlusMinusStat (GS (IE_ACCRUSHINGMOD))))

		RecordsTextArea.Append ("\n\n")

	###################
	# Arcane spell failure
	if GS(IE_LEVELBARD) + GS(IE_LEVELSORCEROR) + GS(IE_LEVELMAGE):
		RecordsTextArea.Append ("[color=ffff00]")
		RecordsTextArea.Append (41391)
		RecordsTextArea.Append ("[/color]\n")

		# Casting Failure
		total = GemRB.GetSpellFailure (pc)
		other = 5*(tohit["Armor"] + tohit["Shield"]) - 5*GemRB.HasFeat(pc, FEAT_ARMORED_ARCANA)
		# account for armored arcana only if it doesn't cause a negative value (eating into the misc bonus)
		verbose = 0
		if other < 0:
			other = total
		else:
			other = total - other
			verbose = 1
		RecordsTextArea.Append (delimited_txt (41390 , ":", str (total)+"%", 0))
		# Armor Penalty (same as for skills and everything else)
		if verbose and tohit["Armor"]:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (39816 , ":", PlusMinusStat(5*tohit["Armor"])+"%", 0))
		# Shield Penalty
		if verbose and tohit["Shield"]:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (39822, ":", PlusMinusStat(5*tohit["Shield"])+"%", 0))
		# Other, just a guess to show the remainder
		if other:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (33548, ":", PlusMinusStat(other)+"%", 0))

		RecordsTextArea.Append ("\n\n")

	###################
	# Weapon Statistics
	RecordsTextArea.Append ("[color=ffff00]")
	RecordsTextArea.Append (41119)
	RecordsTextArea.Append ("[/color]\n")

	slot = GemRB.GetEquippedQuickSlot (pc)
	slot_item = GemRB.GetSlotItem (pc, slot)
	if not slot_item:
		print "ARGHH, no slot item at slot %d (%d), bailing out!" %(slot, combatdet["Slot"])
		return
	item = GemRB.GetItem (slot_item["ItemResRef"])
	ammoslot = GemRB.GetEquippedAmmunition (pc)
	ammo = None
	if ammoslot != -1:
		ammosi = GemRB.GetSlotItem (pc, slot)
		ammo = GemRB.GetItem (ammosi["ItemResRef"])
	print "SAD DEBUG LOG", ammoslot, ammo
	# Main Hand - weapon name
	#  or Ranged - ammo
	if combatdet["Flags"]&15 == 2 and ammo: # this is basically wi.wflags & WEAPON_STYLEMASK == WEAPON_RANGED
		RecordsTextArea.Append (delimited_str (41123, " -", ammo["ItemNameIdentified"], 0))
	else:
		RecordsTextArea.Append (delimited_str (734, " -", item["ItemNameIdentified"], 0))

	# Damage
	# display the unresolved damage string (2d6)
	# this is ammo, launcher details come later
	wdice = combatdet["HitHeaderNumDice"]
	wsides = combatdet["HitHeaderDiceSides"]
	wbonus = combatdet["HitHeaderDiceBonus"]
	AddIndent()
	if wbonus:
		RecordsTextArea.Append (delimited_txt (39518, ":", str (wdice)+"d"+str(wsides)+PlusMinusStat(wbonus), 0))
	else:
		RecordsTextArea.Append (delimited_txt (39518, ":", str (wdice)+"d"+str(wsides), 0))
	# any 100% chance extended headers with damage, eg. Fire: +1d6, which is also computed for the total (00arow08)
	alldos = combatdet["DamageOpcodes"]
	dosmin = 0
	dosmax = 0
	for dos in alldos:
		ddice = dos["NumDice"]
		dsides = dos["DiceSides"]
		dbonus = dos["DiceBonus"]
		AddIndent()
		if dbonus:
			RecordsTextArea.Append (dos["TypeName"] + ": +" + str (ddice)+"d"+str(dsides)+PlusMinusStat(dbonus))
		else:
			RecordsTextArea.Append (dos["TypeName"] + ": +" + str (ddice)+"d"+str(dsides))
		dosmin += ddice + dbonus
		dosmax += ddice*dsides + dbonus

	# Strength
	abonus = combatdet["WeaponStrBonus"]
	if abonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (1145, ":", PlusMinusStat (abonus), 0))
	# Proficiency (bonus)
	pbonus = combatdet["ProfDmgBon"]
	if pbonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32561, ":", PlusMinusStat(pbonus), 0))
	# Launcher
	lbonus = combatdet["LauncherDmgBon"]
	if lbonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (41408, ":", PlusMinusStat(lbonus), 0))
	# Damage Potential (2-12)
	# add any other bonus to the ammo damage calc
	AddIndent()
	wmin = wdice + wbonus + abonus + pbonus + lbonus + dosmin
	wmax = wdice*wsides + wbonus + abonus + pbonus + lbonus + dosmax
	RecordsTextArea.Append (delimited_txt (41120, ":", str (wmin)+"-"+str(wmax), 0))
	# Critical Hit (19-20 / x2)
	crange = combatdet["CriticalRange"]
	cmulti = combatdet["CriticalMultiplier"]
	if crange == 20:
		crange = "20 / x" + str(cmulti)
	else:
		crange = str(crange) + "-20 / x" + str(cmulti)
	AddIndent()
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
	bstr = GUICommon.GetAbilityBonus (pc, IE_STR)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	dint = sint-GemRB.GetPlayerStat (pc, IE_INT,1)
	bint = GUICommon.GetAbilityBonus (pc, IE_INT)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	dwis = swis-GemRB.GetPlayerStat (pc, IE_WIS,1)
	bwis = GUICommon.GetAbilityBonus (pc, IE_WIS)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	ddex = sdex-GemRB.GetPlayerStat (pc, IE_DEX,1)
	bdex = GUICommon.GetAbilityBonus (pc, IE_DEX)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	dcon = scon-GemRB.GetPlayerStat (pc, IE_CON,1)
	bcon = GUICommon.GetAbilityBonus (pc, IE_CON)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	dchr = schr-GemRB.GetPlayerStat (pc, IE_CHR,1)
	bchr = GUICommon.GetAbilityBonus (pc, IE_CHR)

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

	DisplayCommon (pc)

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
