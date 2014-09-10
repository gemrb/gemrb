# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2009 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIREC.py - scripts to control stats/records windows from the GUIREC winpack
###################################################
import GemRB
import GameCheck
import GUICommon
import Spellbook
import CommonTables
import LUCommon
import LevelUp
import GUIWORLD
import DualClass
import GUIRECCommon
from GUIDefines import *
from ie_stats import *
from ie_restype import *
###################################################
RecordsWindow = None
InformationWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
KitInfoWindow = None
ColorTable = None
ColorIndex = None
ScriptTextArea = None
SelectedTextArea = None

###################################################
def OpenRecordsWindow ():
	import GUICommonWindows

	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenRecordsWindow):
		if InformationWindow: OpenInformationWindow ()

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
		GUICommonWindows.UpdatePortraitWindow ()
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIREC", 640, 480)
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)
	# saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenRecordsWindow)
	GUICommonWindows.MarkMenuButton (OptionsWindow)
	OptionsWindow.SetFrame ()
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)

	# dual class
	Button = Window.GetControl (0)
	Button.SetText (7174)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DualClass.DualClassWindow)

	# levelup
	Button = Window.GetControl (37)
	Button.SetText (7175)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LevelUp.OpenLevelUpWindow)

	# information
	Button = Window.GetControl (1)
	Button.SetText (11946)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenInformationWindow)

	# reform party
	Button = Window.GetControl (51)
	Button.SetText (16559)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenRecReformPartyWindow)

	# customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenCustomizeWindow)

	# export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenExportWindow)

	# kit info
	if GameCheck.IsBG2():
		Button = Window.GetControl (52)
		Button.SetText (61265)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenKitInfoWindow)

	GUICommonWindows.SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()

	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

#original returns to game before continuing...
def OpenRecReformPartyWindow ():
	OpenRecordsWindow ()
	GemRB.SetTimedEvent (GUIWORLD.OpenReformPartyWindow, 1)
	return

#don't allow exporting polymorphed or dead characters
def Exportable(pc):
	if not (GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE): return False
	if GemRB.GetPlayerStat (pc, IE_POLYMORPHED): return False
	if GemRB.GetPlayerStat (pc, IE_STATE_ID)&STATE_DEAD: return False
	return True

def UpdateRecordsWindow ():
	global alignment_help

	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	#update mage school
	GemRB.SetVar ("MAGESCHOOL", 0)
	Kit = GUICommon.GetKitIndex (pc)
	if Kit and CommonTables.KitList.GetValue (Kit, 7) == 1:
		MageTable = GemRB.LoadTable ("magesch")
		GemRB.SetVar ("MAGESCHOOL", MageTable.FindValue (3, CommonTables.KitList.GetValue (Kit, 6) ) )

	# exportable
	Button = Window.GetControl (36)
	if Exportable (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# dual-classable
	Button = Window.GetControl (0)
	if GUICommon.CanDualClass (pc):
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	# levelup
	Button = Window.GetControl (37)
	if LUCommon.CanLevelUp (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# name
	Label = Window.GetControl (0x1000000e)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = Window.GetControl (2)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	if (GameCheck.IsBG2() or GameCheck.IsIWD1()) and not GameCheck.IsBG2Demo():
		Button.SetPicture (GemRB.GetPlayerPortrait (pc,0), "NOPORTMD")
	else:
		Button.SetPicture (GemRB.GetPlayerPortrait (pc,0), "NOPORTLG")

	# armorclass
	Label = Window.GetControl (0x10000028)
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label.SetText (str (ac))
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
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	cstr = GetStatColor (pc, IE_STR)
	if sstrx > 0 and sstr==18:
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	else:
		sstr = str (sstr)

	sint = str (GemRB.GetPlayerStat (pc, IE_INT))
	cint = GetStatColor (pc, IE_INT)
	swis = str (GemRB.GetPlayerStat (pc, IE_WIS))
	cwis = GetStatColor (pc, IE_WIS)
	sdex = str (GemRB.GetPlayerStat (pc, IE_DEX))
	cdex = GetStatColor (pc, IE_DEX)
	scon = str (GemRB.GetPlayerStat (pc, IE_CON))
	ccon = GetStatColor (pc, IE_CON)
	schr = str (GemRB.GetPlayerStat (pc, IE_CHR))
	cchr = GetStatColor (pc, IE_CHR)

	Label = Window.GetControl (0x1000002f)
	Label.SetText (sstr)
	Label.SetTextColor (cstr[0], cstr[1], cstr[2])

	Label = Window.GetControl (0x10000009)
	Label.SetText (sdex)
	Label.SetTextColor (cdex[0], cdex[1], cdex[2])

	Label = Window.GetControl (0x1000000a)
	Label.SetText (scon)
	Label.SetTextColor (ccon[0], ccon[1], ccon[2])

	Label = Window.GetControl (0x1000000b)
	Label.SetText (sint)
	Label.SetTextColor (cint[0], cint[1], cint[2])

	Label = Window.GetControl (0x1000000c)
	Label.SetText (swis)
	Label.SetTextColor (cwis[0], cwis[1], cwis[2])

	Label = Window.GetControl (0x1000000d)
	Label.SetText (schr)
	Label.SetTextColor (cchr[0], cchr[1], cchr[2])

	# class
	ClassTitle = GUICommon.GetActorClassTitle (pc)
	Label = Window.GetControl (0x10000030)
	Label.SetText (ClassTitle)

	# race
	text = CommonTables.Races.GetValue (CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (pc, IE_RACE)) , 0)

	Label = Window.GetControl (0x1000000f)
	Label.SetText (text)

	# alignment
	text = CommonTables.Aligns.FindValue (3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT))
	text = CommonTables.Aligns.GetValue (text, 0)
	Label = Window.GetControl (0x10000010)
	Label.SetText (text)

	# gender
	Label = Window.GetControl (0x10000011)
	if GemRB.GetPlayerStat (pc, IE_SEX) == 1:
		Label.SetText (7198)
	else:
		Label.SetText (7199)

	# help, info textarea
	Text = Window.GetControl (45)
	Text.SetText (GetStatOverview (pc))
	#TODO: making window visible/shaded depending on the pc's state
	Window.SetVisible (WINDOW_VISIBLE)
	return

def GetStatColor (pc, stat):
	a = GemRB.GetPlayerStat (pc, stat)
	b = GemRB.GetPlayerStat (pc, stat, 1)
	if a==b:
		return (255,255,255)
	if a<b:
		return (255,255,0)
	return (0,255,0)

# GemRB.GetPlayerStat wrapper that only returns nonnegative values
def GSNN (pc, stat):
	val = GemRB.GetPlayerStat (pc, stat)
	if val >= 0:
		return val
	else:
		return 0

# LevelDiff is used only from the level up code and holds the level
# difference for each class
def GetStatOverview (pc, LevelDiff=[0,0,0]):
	StateTable = GemRB.LoadTable ("statdesc")

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GB = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s, 1)
	GA = lambda s, col, pc=pc: GemRB.GetAbilityBonus (s, col, GS (s) )

	# everyone but bg1 has it somewhere
	if GameCheck.IsBG2():
		str_None = GemRB.GetString (61560)
	elif GameCheck.IsBG1():
		str_None = -1
	elif GameCheck.IsPST():
		str_None = GemRB.GetString (41275)
	else:
		str_None = GemRB.GetString (17093)

	stats = []
	cdet = GemRB.GetCombatDetails(pc, 0)
	tohit = cdet["ToHitStats"]

	# class levels
	# 16480 <CLASS>: Level <LEVEL>
	# Experience: <EXPERIENCE>
	# Next Level: <NEXTLEVEL>

	# collecting tokens for stat overview
	ClassTitle = GUICommon.GetActorClassTitle (pc)
	GemRB.SetToken ("CLASS", ClassTitle)
	Class = GUICommon.GetClassRowName (pc)
	Dual = GUICommon.IsDualClassed (pc, 1)
	Multi = GUICommon.IsMultiClassed (pc, 1)
	XP = GemRB.GetPlayerStat (pc, IE_XP)
	LevelDrain = GS (IE_LEVELDRAIN)

	if GS (IE_STATE_ID) & STATE_DEAD:
		stats.append ( (11829,1,'c') ) # DEAD
		stats.append (None)

	if Multi[0] > 1: # we're multiclassed
		print "\tMulticlassed"
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), GemRB.GetPlayerStat (pc, IE_LEVEL2), GemRB.GetPlayerStat (pc, IE_LEVEL3)]

		stats.append ( (19721,1,'c') )
		stats.append (None)
		for i in range (Multi[0]):
			Class = GUICommon.GetClassRowName (Multi[i+1], "class")
			ClassTitle = GemRB.GetString (CommonTables.Classes.GetValue (Class, "CAP_REF"))
			GemRB.SetToken ("CLASS", ClassTitle)
			GemRB.SetToken ("LEVEL", str (Levels[i]+LevelDiff[i]-int(LevelDrain/Multi[0])) )
			GemRB.SetToken ("EXPERIENCE", str (XP/Multi[0]) )
			if LevelDrain:
				stats.append ( (GemRB.GetString (19720),1,'d') )
				stats.append ( (GemRB.GetString (57435),1,'d') ) # LEVEL DRAINED
			else:
				GemRB.SetToken ("NEXTLEVEL", LUCommon.GetNextLevelExp (Levels[i]+LevelDiff[i], Class) )
				stats.append ( (GemRB.GetString (16480),"",'d') )
			stats.append (None)
			print "\t\tClass (Level):",Class,"(",Levels[i],")"

	elif Dual[0] > 0: # dual classed; first show the new class
		print "\tDual classed"
		stats.append ( (19722,1,'c') )
		stats.append (None)

		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), GemRB.GetPlayerStat (pc, IE_LEVEL2), GemRB.GetPlayerStat (pc, IE_LEVEL3)]

		# the levels are stored in the class order (eg. FIGHTER_MAGE)
		# the current active class does not matter!
		if GUICommon.IsDualSwap (pc):
			Levels = [Levels[1], Levels[0], Levels[2]]

		Levels[0] += LevelDiff[0]

		Class = CommonTables.Classes.GetRowName (Dual[2])
		ClassTitle = GemRB.GetString (CommonTables.Classes.GetValue (Class, "CAP_REF"))
		GemRB.SetToken ("CLASS", ClassTitle)
		GemRB.SetToken ("LEVEL", str (Levels[0]-LevelDrain))
		XP2 = GemRB.GetPlayerStat (pc, IE_XP)
		GemRB.SetToken ("EXPERIENCE", str (XP2) )
		if LevelDrain:
			stats.append ( (GemRB.GetString (19720),1,'d') )
			stats.append ( (GemRB.GetString (57435),1,'d') ) # LEVEL DRAINED
		else:
			GemRB.SetToken ("NEXTLEVEL", LUCommon.GetNextLevelExp (Levels[0], Class) )
			stats.append ( (GemRB.GetString (16480),"",'d') )
		stats.append (None)

		# the first class (shown second)
		if Dual[0] == 1:
			ClassTitle = GemRB.GetString (CommonTables.KitList.GetValue (Dual[1], 2))
		elif Dual[0] == 2:
			ClassTitle = GUICommon.GetClassRowName(Dual[1], "index")
			ClassTitle = GemRB.GetString (CommonTables.Classes.GetValue (ClassTitle, "CAP_REF"))
		GemRB.SetToken ("CLASS", ClassTitle)
		GemRB.SetToken ("LEVEL", str (Levels[1]) )

		# the xp table contains only classes, so we have to determine the base class for kits
		if Dual[0] == 2:
			BaseClass = CommonTables.Classes.GetRowName (Dual[1])
		else:
			BaseClass = GUICommon.GetKitIndex (pc)
			BaseClass = CommonTables.KitList.GetValue (BaseClass, 7)
			BaseClass = GUICommon.GetClassRowName (BaseClass, "class")
		# the first class' XP is discarded and set to the minimum level
		# requirement, so if you don't dual class right after a levelup,
		# the game would eat some of your XP
		XP1 = CommonTables.NextLevel.GetValue (BaseClass, str (Levels[1]))
		GemRB.SetToken ("EXPERIENCE", str (XP1) )

		# inactive until the new class SURPASSES the former
		if Levels[0] <= Levels[1]:
			# inactive
			stats.append ( (19719,1,'c') )
		else:
			stats.append ( (19720,1,'c') )
		stats.append (None)
	else: # single classed
		print "\tSingle classed"
		Level = GemRB.GetPlayerStat (pc, IE_LEVEL) + LevelDiff[0]
		GemRB.SetToken ("LEVEL", str (Level-LevelDrain))
		GemRB.SetToken ("EXPERIENCE", str (XP) )
		if LevelDrain:
			stats.append ( (19720,1,'c') )
			stats.append ( (57435,1,'c') ) # LEVEL DRAINED
		else:
			GemRB.SetToken ("NEXTLEVEL", LUCommon.GetNextLevelExp (Level, Class) )
			stats.append ( (16480,1,'c') )
		stats.append (None)
		print "\t\tClass (Level):",Class,"(",Level,")"

	# check to see if we have a level diff anywhere
	if sum (LevelDiff) == 0:
		effects = GemRB.GetPlayerStates (pc)
		if len (effects):
			for c in effects:
				tmp = StateTable.GetValue (str(ord(c)-66), "DESCRIPTION")
				stats.append ( (tmp,c,'a') )

	#proficiencies
	stats.append ( (8442,1,'c') )

	# look ma, I can use both hands
	
	if GameCheck.IsBG2():
		stats.append ( (61932, tohit["Base"], '0') )
		if (GemRB.IsDualWielding(pc)):
			stats.append ( (56911, tohit["Total"], '0') ) # main
			stats.append ( (56910, GemRB.GetCombatDetails(pc, 1)["ToHitStats"]["Total"], '0') ) # offhand
		else:
			stats.append ( (9457, tohit["Total"], '0') )
	else:
		stats.append ( (9457, str(tohit["Base"])+" ("+str(tohit["Total"])+")", '0') )

	tmp = cdet["APR"]
	if (tmp&1):
		tmp2 = str (tmp/2) + chr (189) #must use one higher than the frame count
	else:
		tmp2 = str (tmp/2)
	stats.append ( (9458, tmp2, '') )
	stats.append ( (9459, GSNN (pc, IE_LORE), '0') )
	if GameCheck.IsBG1() or GameCheck.IsIWD1():
		stats.append ( (19224, GS (IE_RESISTMAGIC), '') )

	# party's reputation
	reptxt = GetReputation (GemRB.GameGetReputation ()/10)
	stats.append ( (9465, reptxt, '') )
	stats.append ( (9460, GSNN (pc, IE_LOCKPICKING), '') )
	stats.append ( (9462, GSNN (pc, IE_TRAPS), '') )
	stats.append ( (9463, GSNN (pc, IE_PICKPOCKET), '') )
	stats.append ( (9461, GSNN (pc, IE_STEALTH), '') )
	HatedRace = GS (IE_HATEDRACE)
	if HatedRace:
		HateTable = GemRB.LoadTable ("haterace")
		Racist = HateTable.FindValue (1, HatedRace)
		if Racist != -1:
			HatedRace = HateTable.GetValue (Racist, 0)
			stats.append ( (15982, GemRB.GetString (HatedRace), '') )

	# these skills were new in bg2
	if GameCheck.IsBG2() or GameCheck.IsIWD1():
		stats.append ( (34120, GSNN (pc, IE_HIDEINSHADOWS), '') )
		stats.append ( (34121, GSNN (pc, IE_DETECTILLUSIONS), '') )
		stats.append ( (34122, GSNN (pc, IE_SETTRAPS), '') )
	stats.append ( (12128, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x') )
	stats.append ( (12126, GS (IE_TURNUNDEADLEVEL), '') )

	#this hack only displays LOH if we know the spell
	#TODO: the core should just not set LOH if the paladin can't learn it
	if (Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, "SPCL211") >= 0):
		stats.append ( (12127, GS (IE_LAYONHANDSAMOUNT), '') )

	#script
	aiscript = GemRB.GetPlayerScript (pc )
	stats.append ( (2078, aiscript, '') )
	stats.append (None)

	# 17379 Saving throws
	stats.append (17379)
	# 17380 Paralyze/Poison/Death
	stats.append ( (17380, IE_SAVEVSDEATH, 's') )
	# 17381 Rod/Staff/Wand
	stats.append ( (17381, IE_SAVEVSWANDS, 's') )
	# 17382 Petrify/Polymorph
	stats.append ( (17382, IE_SAVEVSPOLY, 's') )
	# 17383 Breath weapon
	stats.append ( (17383, IE_SAVEVSBREATH, 's') )
	# 17384 Spells
	stats.append ( (17384, IE_SAVEVSSPELL, 's') )
	stats.append (None)

	# 9466 Weapon proficiencies
	stats.append (9466)
	table = GemRB.LoadTable ("weapprof")
	RowCount = table.GetRowCount ()
	# the first 7 profs are foobared (bg1 style)
	if GameCheck.IsBG2():
		offset = 8
	else:
		offset = 0
	for i in range (offset, RowCount):
		# iwd displays capitalised strings
		# FIXME: ignore it and do the capitalisation manually, so it works for everyone
		if GameCheck.IsIWD1():
			text = table.GetValue (i, 3)
		else:
			text = table.GetValue (i, 1)
		stat = table.GetValue (i, 0)
		if not offset and not GameCheck.IsIWD1(): # TODO: fishy, recheck
			stat = stat + IE_PROFICIENCYBASTARDSWORD
		if text < 0x20000:
			stats.append ( (text, GS (stat)&0x07, '+') )
	stats.append (None)

	# 11766 AC Bonuses
	stats.append (11766)
	# 11770 AC vs. Crushing
	stats.append ((11770, GS (IE_ACCRUSHINGMOD), 'p'))
	# 11767 AC vs. Missile
	stats.append ((11767, GS (IE_ACMISSILEMOD), 'p'))
	# 11769 AC vs. Piercing
	stats.append ((11769, GS (IE_ACPIERCINGMOD), 'p'))
	# 11768 AC vs. Slashing
	stats.append ((11768, GS (IE_ACSLASHINGMOD), 'p'))
	stats.append (None)

	# 10315 Ability bonuses
	stats.append (10315)
	value = GemRB.GetPlayerStat (pc, IE_STR)
	ex = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	# 10332 to hit
	stats.append ( (10332, GemRB.GetAbilityBonus (IE_STR,0,value,ex), 'p') )
	# 10336 damage
	stats.append ( (10336, GemRB.GetAbilityBonus (IE_STR,1,value,ex), 'p') )
	# 10337 open doors (bend bars lift gates)
	stats.append ( (10337, GemRB.GetAbilityBonus (IE_STR,2,value,ex), '0') )
	# 10338 weight allowance
	stats.append ( (10338, GemRB.GetAbilityBonus (IE_STR,3,value,ex), '0') )
	# 10339 AC
	stats.append ( (10339, GA (IE_DEX,2), '0') )
	# 10340 Missile adjustment
	stats.append ( (10340, GA (IE_DEX,1), 'p') )
	# 10341 Reaction adjustment
	stats.append ( (10341, GA (IE_DEX,0), 'p') )
	# 10342 CON HP Bonus/Level
	# dual-classed chars get no bonus while the primary class is inactive
	# and the new class' bonus afterwards
	# single- and multi-classed chars are straightforward - the highest class bonus counts
	if GUICommon.IsWarrior (pc):
		stats.append ( (10342, GA (IE_CON, 1), 'p') )
	else:
		stats.append ( (10342, GA (IE_CON, 0), 'p') )
	# 10343 Chance To Learn spell
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, 0, 0)>0:
		stats.append ( (10343, GA (IE_INT,0), '%' ) )
	# 10347 Reaction
	stats.append ( (10347, GA (IE_REPUTATION,0), '0') )
	stats.append (None)

	# 10344 Bonus Priest spells
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, 0, 0)>0:
		stats.append (10344)
		for level in range (7):
			GemRB.SetToken ("SPELLLEVEL", str (level+1) )
			#get the bonus spell count
			base = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level, 0)
			if base:
				count = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level)
				stats.append ( (GemRB.GetString (10345), count-base, 'r') )
		stats.append (None)

	# only bg2 displayed all the resistances, but it is useful information
	# Resistances
	if GameCheck.IsBG2():
		stats.append(32204)
	elif not GameCheck.IsBG1():
		stats.append (15544)
	# 32213 Normal Fire
	# 32222 Magic Fire
	# 32214 Normal Cold
	# 32223 Magic Cold
	if GameCheck.IsBG2():
		stats.append ((32213, GS (IE_RESISTFIRE), '%'))
		stats.append ((32222, GS (IE_RESISTMAGICFIRE), '%'))
		stats.append ((32214, GS (IE_RESISTCOLD), '%'))
		stats.append ((32223, GS (IE_RESISTMAGICCOLD), '%'))
	elif GameCheck.IsBG1():
		stats.append ((14012, GS (IE_RESISTFIRE), '%'))
		stats.append ((14077, GS (IE_RESISTMAGICFIRE), '%'))
		stats.append ((14014, GS (IE_RESISTCOLD), '%'))
		stats.append ((14078, GS (IE_RESISTMAGICCOLD), '%'))
	else:
		stats.append ((15545, GS (IE_RESISTFIRE), '%'))
		stats.append ((15579, GS (IE_RESISTMAGICFIRE), '%'))
		stats.append ((15546, GS (IE_RESISTCOLD), '%'))
		stats.append ((15580, GS (IE_RESISTMAGICCOLD), '%'))
	# 32220 Electricity
	if GameCheck.IsBG1() or GameCheck.IsIWD1():
		stats.append ((14013, GS (IE_RESISTELECTRICITY), '%'))
	else:
		stats.append ((32220, GS (IE_RESISTELECTRICITY), '%'))
	# 32221 Acid
	if GameCheck.IsBG1() or GameCheck.IsIWD1():
		stats.append ((14015, GS (IE_RESISTACID), '%'))
	else:
		stats.append ((32221, GS (IE_RESISTACID), '%'))
	if GameCheck.IsBG2():
		# Magic (others show it higher up)
		stats.append ((62146, GS (IE_RESISTMAGIC), '%'))
	# Magic Damage
	if GameCheck.IsIWD2():
		stats.append ((40319, GS (IE_MAGICDAMAGERESISTANCE), '%'))
	else: # bg2
		stats.append ((32233, GS (IE_MAGICDAMAGERESISTANCE), '%'))
	# Missile
	stats.append ((11767, GS (IE_RESISTMISSILE), '%'))
	# Slashing
	stats.append ((11768, GS (IE_RESISTSLASHING), '%'))
	# Piercing
	stats.append ((11769, GS (IE_RESISTPIERCING), '%'))
	# Crushing
	stats.append ((11770, GS (IE_RESISTCRUSHING), '%'))
	# Poison
	stats.append ((14017, GS (IE_RESISTPOISON), '%'))
	stats.append (None)

	if GameCheck.IsBG2():
		# Weapon Style bonuses
		stats.append (32131)
		wstyle = cdet["Style"] # equipped weapon style + 1000 * proficiency level
		profcount = wstyle / 1000
		if profcount:
			wstyletables = { IE_PROFICIENCY2WEAPON:"wstwowpn", IE_PROFICIENCY2HANDED:"wstwohnd", IE_PROFICIENCYSINGLEWEAPON:"wssingle", IE_PROFICIENCYSWORDANDSHIELD:"wsshield" }
			bonusrefs = { "THAC0BONUSRIGHT":56911, "THAC0BONUSLEFT":56910, "DAMAGEBONUS":10336, "CRITICALHITBONUS":32140, "PHYSICALSPEED":32141, "AC":10339, "ACVSMISSLE":10340 }
			WStyleTable = GemRB.LoadTable (wstyletables[wstyle%1000])
			for col in range(WStyleTable.GetColumnCount()):
				value = WStyleTable.GetValue (profcount, col)
				stats.append ((bonusrefs[WStyleTable.GetColumnName(col)], value, ''))
		stats.append (None)

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			if type == '+': #pluses
				res.append (GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'p': #a plus prefix if positive
				if val > 0:
					res.append (GemRB.GetString (strref) + ' +' + str (val) )
				else:
					res.append (GemRB.GetString (strref) + ' ' + str (val) )
			elif type == 'r': #a plus prefix if positive, strref is an already resolved string
				if val > 0:
					res.append (strref + ' +' + str (val) )
				else:
					res.append (strref + ' ' + str (val) )
			elif type == 's': #both base and (modified) stat, but only if they differ
				base = GB (val)
				stat = GS (val)
				base_str = GemRB.GetString (strref) + ': ' + str(stat)
				if base == stat:
					res.append (base_str)
				else:
					res.append (base_str + " (" + str(stat-base) + ")")
			elif type == 'x': #x character before value
				res.append (GemRB.GetString (strref) +': x' + str (val) )
			elif type == 'a': #value (portrait icon) + string
				# '%' is the separator glyph in the states font
				res.append ("[cap]" + val + "%[/cap][p]" + GemRB.GetString (strref) + "[/p]")
			elif type == 'b': #strref is an already resolved string
				res.append (strref+": "+str (val))
			elif type == 'c': #normal string
				res.append (GemRB.GetString (strref))
			elif type == 'd': #strref is an already resolved string
				res.append (strref)
			elif type == '0': #normal value
				res.append (GemRB.GetString (strref) + ': ' + str (val))
			else: #normal value + type character, for example percent sign
				res.append (GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append (GemRB.GetString (s) )
				lines = 0
			else:
				if not lines and str_None != -1:
					res.append (str_None)
				res.append ("")
				lines = 0

	return "\n".join (res)

def GetReputation (repvalue):
	table = GemRB.LoadTable ("reptxt")
	if repvalue>20:
		repvalue=20
	txt = GemRB.GetString (table.GetValue (repvalue, 0) )
	return txt+" ("+str (repvalue)+")"

def OpenInformationWindow ():
	global InformationWindow

	if InformationWindow != None:
		GUIRECCommon.CloseBiographyWindow ()
		CloseInformationWindow ()
		return

	InformationWindow = Window = GemRB.LoadWindow (4)

	# Biography
	Button = Window.GetControl (26)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenBiographyWindow)

	# Done
	Button = Window.GetControl (24)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseInformationWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	TotalPartyExp = 0
	ChapterPartyExp = 0
	TotalPartyKills = 0
	ChapterCount = 0
	for i in range (1, GemRB.GetPartySize () + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsTotalXP']
		ChapterPartyExp = ChapterPartyExp + stat['KillsChapterXP']
		TotalPartyKills = TotalPartyKills + stat['KillsTotalCount']
		ChapterCount = ChapterCount + stat['KillsChapterCount']

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()
	stat = GemRB.GetPCStats (pc)

	Label = Window.GetControl (0x10000000)
	Label.SetText (GemRB.GetPlayerName (pc, 1))
	# class
	ClassTitle = GUICommon.GetActorClassTitle (pc)
	Label = Window.GetControl (0x10000018)
	Label.SetText (ClassTitle)

	#most powerful vanquished
	Label = Window.GetControl (0x10000005)
	#we need getstring, so -1 will translate to empty string
	Label.SetText (GemRB.GetString (stat['BestKilledName']))

	Label = Window.GetControl (0x10000006)
	GUICommon.SetCurrentDateTokens (stat)
	#actually it is 16043 <DURATION>, but duration is translated to
	#16041, hopefully this won't cause problem with international version
	Label.SetText (16041)

	#favourite spell
	Label = Window.GetControl (0x10000007)
	Label.SetText (stat['FavouriteSpell'])

	#favourite weapon
	Label = Window.GetControl (0x10000008)
	#actually it is 10479 <WEAPONNAME>, but weaponname is translated to
	#the real weapon name (which we should set using SetToken)
	#there are other strings like bow+wname/xbow+wname/sling+wname
	#are they used?
	Label.SetText (stat['FavouriteWeapon'])

	#total party xp
	Label = Window.GetControl (0x10000013)
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	# chapter party xp
	Label = Window.GetControl (0x1000000f)
	if ChapterPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / ChapterPartyExp)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	# total kills
	Label = Window.GetControl (0x10000014)
	if TotalPartyKills != 0:
		PartyKills = int ((stat['KillsTotalCount'] * 100) / TotalPartyKills)
		Label.SetText (str (PartyKills) + '%')
	else:
		Label.SetText ("0%")

	# chapter kills
	Label = Window.GetControl (0x10000010)
	if ChapterCount != 0:
		PartyKills = int ((stat['KillsChapterCount'] * 100) / ChapterCount)
		Label.SetText (str (PartyKills) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000011)
	Label.SetText (str (stat['KillsChapterXP']))
	Label = Window.GetControl (0x10000015)
	Label.SetText (str (stat['KillsTotalXP']))

	#count of kills in chapter/game
	Label = Window.GetControl (0x10000012)
	Label.SetText (str (stat['KillsChapterCount']))
	Label = Window.GetControl (0x10000016)
	Label.SetText (str (stat['KillsTotalCount']))

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseInformationWindow ():
	import GUICommonWindows
	global InformationWindow

	if InformationWindow:
		InformationWindow.Unload ()
	InformationWindow = None

	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	RecordsWindow.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def OpenKitInfoWindow ():
	global KitInfoWindow

	KitInfoWindow = GemRB.LoadWindow (24)

	#back button (setting first, to be less error prone)
	DoneButton = KitInfoWindow.GetControl (2)
	DoneButton.SetText (11973)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, KitDonePress)

	#kit or class description
	TextArea = KitInfoWindow.GetControl (0)

	pc = GemRB.GameGetSelectedPCSingle ()
	ClassName = GUICommon.GetClassRowName (pc)
	Multi = GUICommon.HasMultiClassBits (pc)
	Dual = GUICommon.IsDualClassed (pc, 1)
	text = ""
	if Multi and Dual[0] == 0: # true multi class
		text = CommonTables.Classes.GetValue (ClassName, "DESC_REF")
	else:
		KitIndex = GUICommon.GetKitIndex (pc)

		if Dual[0]: # dual class
			# first (previous) kit or class of the dual class
			if Dual[0] == 1:
				text = CommonTables.KitList.GetValue (Dual[1], 3)
			elif Dual[0] == 2:
				text = CommonTables.Classes.GetValue (GUICommon.GetClassRowName(Dual[1], "index"), "DESC_REF")
	
			text += "\n"
			text += CommonTables.Classes.GetValue (GUICommon.GetClassRowName(Dual[2], "index"), "DESC_REF")
	
		else: # ordinary class or kit
			if KitIndex:
				text = CommonTables.KitList.GetValue (KitIndex, 3)
			else:
				text = CommonTables.Classes.GetValue (ClassName, "DESC_REF")

	TextArea.SetText ("[p]" + text + "[/p]")

	KitInfoWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def KitDonePress():
	if KitInfoWindow:
		KitInfoWindow.Unload()
	return

def OpenColorWindow ():
	global PortraitWindow
	global PaperdollButton
	global HairButton, SkinButton, MajorButton, MinorButton
	global HairColor, SkinColor, MajorColor, MinorColor

	pc = GemRB.GameGetSelectedPCSingle ()
	MinorColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	MajorColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	SkinColor = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	HairColor = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	GUIRECCommon.SubCustomizeWindow = GemRB.LoadWindow (21)

	PaperdollButton = GUIRECCommon.SubCustomizeWindow.GetControl (0)
	PaperdollButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PaperdollButton.SetState (IE_GUI_BUTTON_LOCKED)

	HairButton = GUIRECCommon.SubCustomizeWindow.GetControl (3)
	SkinButton = GUIRECCommon.SubCustomizeWindow.GetControl (4)
	MajorButton = GUIRECCommon.SubCustomizeWindow.GetControl (5)
	MinorButton = GUIRECCommon.SubCustomizeWindow.GetControl (6)

	DoneButton = GUIRECCommon.SubCustomizeWindow.GetControl (12)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = GUIRECCommon.SubCustomizeWindow.GetControl (13)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	HairButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SetHairColor)
	SkinButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SetSkinColor)
	MajorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SetMajorColor)
	MinorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SetMinorColor)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DoneColorWindow)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.CloseSubCustomizeWindow)
	UpdatePaperDoll ()

	GUIRECCommon.SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def DoneColorWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerStat (pc, IE_MINOR_COLOR, MinorColor)
	GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, MajorColor)
	GemRB.SetPlayerStat (pc, IE_SKIN_COLOR, SkinColor)
	GemRB.SetPlayerStat (pc, IE_HAIR_COLOR, HairColor)
	GUIRECCommon.CloseSubCustomizeWindow ()
	return

def UpdatePaperDoll ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	MinorButton.SetBAM ("COLGRAD", 0, 0, MinorColor&0xff)
	MajorButton.SetBAM ("COLGRAD", 0, 0, MajorColor&0xff)
	SkinButton.SetBAM ("COLGRAD", 0, 0, SkinColor&0xff)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	HairButton.SetBAM ("COLGRAD", 0, 0, HairColor&0xff)
	PaperdollButton.SetPLT (GUICommon.GetActorPaperDoll (pc),
		Color1, MinorColor, MajorColor, SkinColor, Color5, Color6, HairColor, 0, 0)
	return

def SetHairColor ():
	global ColorIndex, PickedColor

	ColorIndex = 0
	PickedColor = HairColor
	OpenColorPicker ()
	return

def SetSkinColor ():
	global ColorIndex, PickedColor

	ColorIndex = 1
	PickedColor = SkinColor
	OpenColorPicker ()
	return

def SetMinorColor ():
	global ColorIndex, PickedColor

	ColorIndex = 2
	PickedColor = MinorColor
	OpenColorPicker ()
	return

def SetMajorColor ():
	global ColorIndex, PickedColor

	ColorIndex = 3
	PickedColor = MajorColor
	OpenColorPicker ()
	return

def OpenColorPicker ():
	GUIRECCommon.SubSubCustomizeWindow = GemRB.LoadWindow (22)

	GemRB.SetVar ("Selected",-1)
	for i in range (1,35):
		Button = GUIRECCommon.SubSubCustomizeWindow.GetControl (i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)

	for i in range (34):
		MyColor = ColorTable.GetValue (ColorIndex, i)
		if MyColor == "*":
			break
		Button = GUIRECCommon.SubSubCustomizeWindow.GetControl (i+1)
		Button.SetBAM("COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar ("Selected",i)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("Selected",i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonePress)

	GUIRECCommon.SubSubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def DonePress():
	global HairColor, SkinColor, MajorColor, MinorColor
	global PickedColor

	GUIRECCommon.CloseSubSubCustomizeWindow ()
	PickedColor=ColorTable.GetValue (ColorIndex, GemRB.GetVar ("Selected"))
	if ColorIndex==0:
		HairColor=PickedColor
		UpdatePaperDoll ()
		return
	if ColorIndex==1:
		SkinColor=PickedColor
		UpdatePaperDoll ()
		return
	if ColorIndex==2:
		MinorColor=PickedColor
		UpdatePaperDoll ()
		return

	MajorColor=PickedColor
	UpdatePaperDoll ()
	return

###################################################
# End of file GUIREC.py
