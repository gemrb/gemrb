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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUIREC.py - scripts to control stats/records windows from the GUIREC winpack

###################################################
import string
import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *
from GUIWORLD import OpenReformPartyWindow

###################################################
RecordsWindow = None
InformationWindow = None
BiographyWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
ExportWindow = None
KitInfoWindow = None
LevelUpWindow = None
ExportDoneButton = None
ExportFileName = ""

###################################################
def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenRecordsWindow):
		if InformationWindow: OpenInformationWindow ()

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

	GemRB.LoadWindowPack ("GUIREC", 640, 480)
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow(0)
	MarkMenuButton (OptionsWindow)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenRecordsWindow")
	GemRB.SetWindowFrame (OptionsWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow(0)

	# dual class
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 7174)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DualClassWindow")

	# levelup
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetText (Window, Button, 7175)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LevelUpWindow")

	# information
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 11946)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")

	# reform party
	Button = GemRB.GetControl (Window, 51)
	GemRB.SetText (Window, Button, 16559)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	# customize
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetText (Window, Button, 10645)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CustomizeWindow")

	# export
	Button = GemRB.GetControl (Window, 36)
	GemRB.SetText (Window, Button, 13956)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenExportWindow")

	# kit info
	Button = GemRB.GetControl (Window, 52)
	GemRB.SetText (Window, Button, 61265)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "KitInfoWindow")

	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 3)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def GetNextLevelExp (Level, Class):
	NextLevelTable = GemRB.LoadTable ("XPLEVEL")
	Row = GemRB.GetTableRowIndex (NextLevelTable, Class)
	if Level < GemRB.GetTableColumnCount (NextLevelTable, Row):
		return str(GemRB.GetTableValue (NextLevelTable, Row, Level) )

	return 0

def UpdateRecordsWindow ():
	global stats_overview, alignment_help

	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	# exportable
	Button = GemRB.GetControl (Window, 36)
	if GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# dual-classable
	Button = GemRB.GetControl (Window, 0)
	if CanDualClass (pc):
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	# name
	Label = GemRB.GetControl (Window, 0x1000000e)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture (Window, Button, GemRB.GetPlayerPortrait (pc,0), "NOPORTMD")

	# armorclass
	Label = GemRB.GetControl (Window, 0x10000028)
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	#This is a temporary solution, the core engine should set the stat correctly!
	ac += GemRB.GetAbilityBonus(IE_DEX, 2, GemRB.GetPlayerStat(pc, IE_DEX) )
	GemRB.SetText (Window, Label, str (ac))
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
	cstr = GetStatColor(pc, IE_STR)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	if sstrx > 0 and sstr==18:
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	else:
		sstr = str(sstr)
	sint = str(GemRB.GetPlayerStat (pc, IE_INT))
	cint = GetStatColor(pc, IE_INT)
	swis = str(GemRB.GetPlayerStat (pc, IE_WIS))
	cwis = GetStatColor(pc, IE_WIS)
	sdex = str(GemRB.GetPlayerStat (pc, IE_DEX))
	cdex = GetStatColor(pc, IE_DEX)
	scon = str(GemRB.GetPlayerStat (pc, IE_CON))
	ccon = GetStatColor(pc, IE_CON)
	schr = str(GemRB.GetPlayerStat (pc, IE_CHR))
	cchr = GetStatColor(pc, IE_CHR)

	Label = GemRB.GetControl (Window, 0x1000002f)
	GemRB.SetText (Window, Label, sstr)
	GemRB.SetLabelTextColor (Window, Label, cstr[0], cstr[1], cstr[2])

	Label = GemRB.GetControl (Window, 0x10000009)
	GemRB.SetText (Window, Label, sdex)
	GemRB.SetLabelTextColor (Window, Label, cdex[0], cdex[1], cdex[2])

	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, scon)
	GemRB.SetLabelTextColor (Window, Label, ccon[0], ccon[1], ccon[2])

	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, sint)
	GemRB.SetLabelTextColor (Window, Label, cint[0], cint[1], cint[2])

	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, swis)
	GemRB.SetLabelTextColor (Window, Label, cwis[0], cwis[1], cwis[2])

	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, schr)
	GemRB.SetLabelTextColor (Window, Label, cchr[0], cchr[1], cchr[2])

	# class
	ClassTitle = GetActorClassTitle (pc)
	Label = GemRB.GetControl (Window, 0x10000030)
	GemRB.SetText (Window, Label, ClassTitle)

	# race
	Table = GemRB.LoadTable ("races")
	text = GemRB.GetTableValue (Table, GemRB.GetPlayerStat (pc, IE_RACE) - 1, 0)
	GemRB.UnloadTable (Table)

	Label = GemRB.GetControl (Window, 0x1000000f)
	GemRB.SetText (Window, Label, text)

	Table = GemRB.LoadTable ("aligns")

	text = GemRB.GetTableValue (Table, GemRB.FindTableValue ( Table, 3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT) ), 0)
	GemRB.UnloadTable (Table)
	Label = GemRB.GetControl (Window, 0x10000010)
	GemRB.SetText (Window, Label, text)

	Label = GemRB.GetControl (Window, 0x10000011)
	if GemRB.GetPlayerStat (pc, IE_SEX) == 1:
		GemRB.SetText (Window, Label, 7198)
	else:
		GemRB.SetText (Window, Label, 7199)

	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = GemRB.GetControl (Window, 45)
	GemRB.SetText (Window, Text, stats_overview)
	#making window visible/shaded depending on the pc's state
	GemRB.SetVisible (Window, 1)
	return

def GetStatColor (pc, stat):
	a = GemRB.GetPlayerStat(pc, stat)
	b = GemRB.GetPlayerStat(pc, stat, 1)
	if a==b:
		return (255,255,255)
	if a<b:
		return (255,255,0)
	return (0,255,0)

def GetStatOverview (pc):
	StateTable = GemRB.LoadTable ("statdesc")
	str_None = GemRB.GetString (61560)

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GA = lambda s, col, pc=pc: GemRB.GetAbilityBonus (s, col, GS (s) )

	stats = []
	# class levels
	# 16480 <CLASS>: Level <LEVEL>
	# Experience: <EXPERIENCE>
	# Next Level: <NEXTLEVEL>

	#collecting tokens for stat overview
	ClassTitle = GetActorClassTitle (pc)
	GemRB.SetToken("CLASS", ClassTitle)
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	Class = GemRB.FindTableValue (ClassTable, 5, Class)
	Multi = GemRB.GetTableValue (ClassTable, Class, 4)
	Class = GemRB.GetTableRowName (ClassTable, Class)
	Dual = IsDualClassed (pc, 1)
	if Multi:
		Levels = [IE_LEVEL, IE_LEVEL2, IE_LEVEL3]
		Classes = [0,0,0]
		MultiCount = 0
		Mask = 1
		for i in range (16):
			if Multi&Mask:
				Classes[MultiCount] = GemRB.FindTableValue (ClassTable, 5, i+1)
				MultiCount += 1
			Mask += Mask

		if Dual[0] > 0: # dual classed
			stats.append ( (19722,1,'c') )

			if Dual[0] == 1:
				ClassTitle = GemRB.GetString(GemRB.GetTableValue (KitTable, Dual[1], 2))
			elif Dual[0] == 2:
				ClassTitle = GemRB.GetString(GemRB.GetTableValue (ClassTable, Dual[1], 2))
			GemRB.SetToken("CLASS", ClassTitle)

			Level = GemRB.GetPlayerStat (pc, Levels[1])
			GemRB.SetToken("LEVEL", str (Level) )

			# the xp table contains only classes
			XPTable = GemRB.LoadTable ("xplevel")
			if Dual[0] == 2:
				XP1 = GemRB.GetTableRowName (XPTable, Dual[1])
			else:
				KitTable = GemRB.LoadTable ("kitlist")
				BaseClass = GetKitIndex (pc)
				BaseClass = GemRB.GetTableValue (KitTable, BaseClass, 7)
				BaseClass = GemRB.GetTableRowName (ClassTable, BaseClass)
				XP1 = GemRB.GetTableRowName (XPTable, BaseClass)
			# the first class' XP is discarded and set to the minimum level 
			# requirement, so if you don't dual class right after a levelup, 
			# the game would eat some of your XP
			XP1 = GemRB.GetTableValue (XPTable, XP1, str(Level))
			GemRB.SetToken("EXPERIENCE", str (XP1) )

			stats.append ( (GemRB.GetString(19720),"",'d') )
			stats.append (None)

			# the second class
			ClassTitle = GemRB.GetString(GemRB.GetTableValue (ClassTable, Dual[2], 2))
			GemRB.SetToken("CLASS", ClassTitle)
			Class = GemRB.GetTableRowName (ClassTable, Dual[2])

			Level = GemRB.GetPlayerStat (pc, Levels[0])
			GemRB.SetToken("LEVEL", str (Level) )
			GemRB.SetToken("NEXTLEVEL", GetNextLevelExp (Level, Class) )

			# remove the first class's XP from IE_XP
			XP2 = GemRB.GetPlayerStat (pc, IE_XP) - XP1
			GemRB.SetToken("EXPERIENCE", str (XP2) )
			stats.append ( (16480,1,'c') )
			stats.append (None)
		else: # multi classed
			stats.append ( (19721,1,'c') )
			for i in range (MultiCount):
				Class = Classes[i]
				ClassTitle = GemRB.GetString(GemRB.GetTableValue (ClassTable, Class, 2))
				GemRB.SetToken("CLASS", ClassTitle)
				Class = GemRB.GetTableRowName (ClassTable, i)
				Level = GemRB.GetPlayerStat (pc, Levels[i])
				GemRB.SetToken("LEVEL", str (Level) )
				GemRB.SetToken("NEXTLEVEL", GetNextLevelExp (Level, Class) )
				GemRB.SetToken("EXPERIENCE", str (GemRB.GetPlayerStat (pc, IE_XP)/MultiCount ) )
				#resolve string immediately
				stats.append ( (GemRB.GetString(16480),"",'d') )
				stats.append (None)

	else: # single classed
		Level = GemRB.GetPlayerStat (pc, IE_LEVEL)
		GemRB.SetToken("LEVEL", str (Level) )
		GemRB.SetToken("NEXTLEVEL", GetNextLevelExp (Level, Class) )
		GemRB.SetToken("EXPERIENCE", str (GemRB.GetPlayerStat (pc, IE_XP) ) )
		stats.append ( (16480,1,'c') )
		stats.append (None)
	GemRB.UnloadTable (ClassTable)

	effects = GemRB.GetPlayerStates (pc)
	if len(effects):
		for c in effects:
			tmp = GemRB.GetTableValue (StateTable, ord(c)-66, 0)
			stats.append ( (tmp,c,'a') )
		stats.append (None)

	stats.append ( (8442,1,'c') )

	stats.append ( (61932, GS (IE_THAC0), '') )
	stats.append ( (9457, GS (IE_THAC0), '') )
	tmp = GS (IE_NUMBEROFATTACKS)
	if (tmp&1):
		tmp2 = str(tmp/2) + chr(188)
	else:
		tmp2 = str(tmp/2)
	stats.append ( (9458, tmp2, '') )
	stats.append ( (9459, GS (IE_LORE), '') )
	reptxt = GetReputation (GemRB.GameGetReputation()/10)
	stats.append ( (9465, reptxt, '') )
	stats.append ( (9460, GS (IE_LOCKPICKING), '') )
	stats.append ( (9462, GS (IE_TRAPS), '') )
	stats.append ( (9463, GS (IE_PICKPOCKET), '') )
	stats.append ( (9461, GS (IE_STEALTH), '') )
	stats.append ( (34120, GS (IE_HIDEINSHADOWS), '') )
	stats.append ( (34121, GS (IE_DETECTILLUSIONS), '') )
	stats.append ( (34122, GS (IE_SETTRAPS), '') )
	stats.append ( (12128, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x') )
	stats.append ( (12126, GS (IE_TURNUNDEADLEVEL), '') )
	stats.append ( (12127, GS (IE_LAYONHANDSAMOUNT), '') )
	#script
	aiscript = GemRB.GetPlayerScript (pc )
	stats.append ( (2078, aiscript, '') )
	stats.append (None)


	#   17379 Saving Throws
	stats.append (17379)
	#   17380 Death
	stats.append ( (17380, GS (IE_SAVEVSDEATH), '') )
	#   17381
	stats.append ( (17381, GS (IE_SAVEVSWANDS), '') )
	#   17382 AC vs. Crushing
	stats.append ( (17382, GS (IE_SAVEVSPOLY), '') )
	#   17383 Rod
	stats.append ( (17383, GS (IE_SAVEVSBREATH), '') )
	#   17384 Spells
	stats.append ( (17384, GS (IE_SAVEVSSPELL), '') )
	stats.append (None)

	# 9466 Weapon profs
	stats.append (9466)
	table = GemRB.LoadTable("weapprof")
	RowCount = GemRB.GetTableRowCount (table)
	# the first 7 profs are foobared
	for i in range(8,RowCount):
		text = GemRB.GetTableValue (table, i, 1)
		stat = GemRB.GetTableValue (table, i, 0)
		stats.append ( (text, GS(stat), '+') )
	stats.append (None)

	# 11766 AC Bonuses
	stats.append (11766)
	#   11770 AC vs. Crushing
	stats.append ((11770, GS (IE_ACCRUSHINGMOD), ''))
	#   11767 AC vs. Missile
	stats.append ((11767, GS (IE_ACMISSILEMOD), ''))
	#   11769 AC vs. Piercing
	stats.append ((11769, GS (IE_ACPIERCINGMOD), ''))
	#   11768 AC vs. Slashing
	stats.append ((11768, GS (IE_ACSLASHINGMOD), ''))
	stats.append (None)

	# 10315 Ability bonuses
	stats.append (10315)
	value = GemRB.GetPlayerStat (pc, IE_STR)
	ex = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	# 10332 to hit
	stats.append ( (10332, GemRB.GetAbilityBonus(IE_STR,0,value,ex), '0') )
	# 10336 damage
	stats.append ( (10336, GemRB.GetAbilityBonus(IE_STR,1,value,ex), '0') )
	# 10337 open doors (bend bars lift gates)
	stats.append ( (10337, GemRB.GetAbilityBonus(IE_STR,2,value,ex), '0') )
	# 10338 weight allowance
	stats.append ( (10338, GemRB.GetAbilityBonus(IE_STR,3,value,ex), '0') )
	# 10339 AC
	stats.append ( (10339, GA(IE_DEX,2), '0') )
	# 10340 Missile
	stats.append ( (10340, GA(IE_DEX,1), '0') )
	# 10341 Reaction
	stats.append ( (10341, GA(IE_DEX,0), '0') )
	# 10342 Hp/Level
	stats.append ( (10342, GA(IE_CON,0), '0') )
	# 10343 Chance To Learn spell
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, 0, 0)>0:
		stats.append ((10343, GA (IE_INT,0), '%' ))
	# 10347 Reaction
	stats.append ( (10347, GA(IE_CHR,0), '0') )
	stats.append (None)

	#Bonus spells
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, 0, 0)>0:
		stats.append (10344)
		for level in range(7):
			GemRB.SetToken ("SPELLLEVEL", str(level+1) )
			#get the bonus spell count
			base = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level, 0)
			if base:
				count = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level)
				stats.append ( (GemRB.GetString(10345), count-base, 'b') )
		stats.append (None)

	# 32204 Resistances
	stats.append (32204)
	#   32213 Normal Fire
	stats.append ((32213, GS (IE_RESISTFIRE), '%'))
	#   32222 Magic Fire
	stats.append ((32222, GS (IE_RESISTMAGICFIRE), '%'))
	#   32214 Normal Cold
	stats.append ((32214, GS (IE_RESISTCOLD), '%'))
	#   32223 Magic Cold
	stats.append ((32223, GS (IE_RESISTMAGICCOLD), '%'))
	#   32220 Electricity
	stats.append ((32220, GS (IE_RESISTELECTRICITY), '%'))
	#   32221 Acid
	stats.append ((32221, GS (IE_RESISTACID), '%'))
	#   32233 Magic Damage
	stats.append ((32233, GS (IE_RESISTMAGIC), '%'))
	#   67216 Slashing Attacks
	stats.append ((67216, GS (IE_RESISTSLASHING), '%'))
	#   67217 Piercing Attacks
	stats.append ((67217, GS (IE_RESISTPIERCING), '%'))
	#   67218 Crushing Attacks
	stats.append ((67218, GS (IE_RESISTCRUSHING), '%'))
	#   67219 Missile Attacks
	stats.append ((67219, GS (IE_RESISTMISSILE), '%'))
	stats.append (None)

	#Weapon Style bonuses
	stats.append (32131)
	#
	stats.append (None)

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			if type == '+': #pluses
				res.append ("[capital=0]"+GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'x': #x character before value
				res.append ("[capital=0]"+GemRB.GetString (strref)+': x' + str (val) )
			elif type == 'a': #value (portrait icon) + string
				res.append ("[capital=2]"+val+" "+GemRB.GetString (strref))
			elif type == 'b': #strref is an already resolved string
				res.append ("[capital=0]"+strref+": "+str(val) )
			elif type == 'c': #normal string
				res.append ("[capital=0]"+GemRB.GetString (strref) )
			elif type == 'd': #strref is an already resolved string
				res.append ("[capital=0]"+strref)
			elif type == '0': #normal value
				res.append (GemRB.GetString (strref) + ': ' + str (val) )
			else: #normal value + type character, for example percent sign
				res.append (GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append ("[capital=0]"+ GemRB.GetString (s) )
				lines = 0
			else:
				if not lines:
					res.append (str_None)
				res.append ("")
				lines = 0

	return string.join (res, "\n")


def GetReputation (repvalue):
	table = GemRB.LoadTable ("reptxt")
	if repvalue>20:
		repvalue=20
	txt = GemRB.GetString (GemRB.GetTableValue (table, repvalue, 0) )
	GemRB.UnloadTable (table)
	return txt+"("+str(repvalue)+")"


def OpenInformationWindow ():
	global InformationWindow

	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		GemRB.UnloadWindow (InformationWindow)
		GemRB.SetVisible (OptionsWindow, 3)
		GemRB.SetVisible (RecordsWindow, 3)
		GemRB.SetVisible (PortraitWindow, 3)
		InformationWindow = None

		return

	InformationWindow = Window = GemRB.LoadWindow (4)

	# Biography
	Button = GemRB.GetControl (Window, 26)
	GemRB.SetText (Window, Button, 18003)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 24)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")

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

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()
	stat = GemRB.GetPCStats (pc)

	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))
	# class
	ClassTitle = GetActorClassTitle(pc)
	Label = GemRB.GetControl (Window, 0x10000018)
	GemRB.SetText (Window, Label, ClassTitle)

	#most powerful vanquished
	Label = GemRB.GetControl (Window, 0x10000005)
	#we need getstring, so -1 will translate to empty string
	GemRB.SetText (Window, Label, GemRB.GetString (stat['BestKilledName']))

	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates???). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15)
	print "CurrentTime",currentTime
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	GemRB.SetToken ('GAMEDAY', str (days))
	GemRB.SetToken ('HOUR', str (hours))
	Label = GemRB.GetControl (Window, 0x10000006)
	#actually it is 16043 <DURATION>, but duration is translated to
	#16041, hopefully this won't cause problem with international version
	GemRB.SetText (Window, Label, 16041)

	#favourite spell
	Label = GemRB.GetControl (Window, 0x10000007)
	GemRB.SetText (Window, Label, stat['FavouriteSpell'])

	#favourite weapon
	Label = GemRB.GetControl (Window, 0x10000008)
	#actually it is 10479 <WEAPONNAME>, but weaponname is translated to
	#the real weapon name (which we should set using SetToken)
	#there are other strings like bow+wname/xbow+wname/sling+wname
	#are they used?
	GemRB.SetText (Window, Label, stat['FavouriteWeapon'])

	#total xp
	Label = GemRB.GetControl (Window, 0x1000000f)
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		GemRB.SetText (Window, Label, str (PartyExp) + '%')
	else:
		GemRB.SetText (Window, Label, "0%")

	Label = GemRB.GetControl (Window, 0x10000013)
	if ChapterPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / ChapterPartyExp)
		GemRB.SetText (Window, Label, str (PartyExp) + '%')
	else:
		GemRB.SetText (Window, Label, "0%")

	#total xp
	Label = GemRB.GetControl (Window, 0x10000010)
	if TotalCount != 0:
		PartyExp = int ((stat['KillsTotalCount'] * 100) / TotalCount)
		GemRB.SetText (Window, Label, str (PartyExp) + '%')
	else:
		GemRB.SetText (Window, Label, "0%")

	Label = GemRB.GetControl (Window, 0x10000014)
	if ChapterCount != 0:
		PartyExp = int ((stat['KillsChapterCount'] * 100) / ChapterCount)
		GemRB.SetText (Window, Label, str (PartyExp) + '%')
	else:
		GemRB.SetText (Window, Label, "0%")

	Label = GemRB.GetControl (Window, 0x10000011)
	GemRB.SetText (Window, Label, str (stat['KillsChapterXP']))
	Label = GemRB.GetControl (Window, 0x10000015)
	GemRB.SetText (Window, Label, str (stat['KillsTotalXP']))

	#count of kills in chapter/game
	Label = GemRB.GetControl (Window, 0x10000012)
	GemRB.SetText (Window, Label, str (stat['KillsChapterCount']))
	Label = GemRB.GetControl (Window, 0x10000016)
	GemRB.SetText (Window, Label, str (stat['KillsTotalCount']))

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OpenBiographyWindow ():
	global BiographyWindow

	if BiographyWindow != None:
		GemRB.UnloadWindow (BiographyWindow)
		BiographyWindow = None
		GemRB.ShowModal (InformationWindow, MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)

	TextArea = GemRB.GetControl (Window, 0)
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetText (Window, TextArea, GemRB.GetPlayerString(pc, 74) )

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OpenExportWindow ():
	global ExportWindow, NameField, ExportDoneButton

	ExportWindow = GemRB.LoadWindow(13)

	TextArea = GemRB.GetControl(ExportWindow, 2)
	GemRB.SetText(ExportWindow, TextArea, 10962)

	TextArea = GemRB.GetControl(ExportWindow,0)
	GemRB.GetCharacters (ExportWindow, TextArea)

	ExportDoneButton = GemRB.GetControl(ExportWindow, 4)
	GemRB.SetText(ExportWindow, ExportDoneButton, 11973)
	GemRB.SetButtonState(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(ExportWindow,5)
	GemRB.SetText(ExportWindow, CancelButton, 13727)

	NameField = GemRB.GetControl(ExportWindow,6)

	GemRB.SetEvent(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_ON_PRESS, "ExportDonePress")
	GemRB.SetEvent(ExportWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExportCancelPress")
	GemRB.SetEvent(ExportWindow, NameField, IE_GUI_EDIT_ON_CHANGE, "ExportEditChanged")
	GemRB.ShowModal (ExportWindow, MODAL_SHADOW_GRAY)
	GemRB.SetControlStatus (ExportWindow, NameField,IE_GUI_CONTROL_FOCUSED)
	return

def ExportDonePress():
	GemRB.UnloadWindow(ExportWindow)
	#save file under name from EditControl
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SaveCharacter(pc, ExportFileName)
	return

def ExportCancelPress():
	GemRB.UnloadWindow(ExportWindow)
	return

def ExportEditChanged():
	global ExportFileName

	ExportFileName = GemRB.QueryText(ExportWindow, NameField)
	if ExportFileName == "":
		GemRB.SetButtonState(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def CanDualClass(actor):
	# human
	if GemRB.GetPlayerStat (actor, IE_RACE) != 1:
		return 1

	# already dualclassed
	Dual = IsDualClassed (actor,0)
	if Dual[0] > 0:
		return 1

	DualClassTable = GemRB.LoadTable ("dualclas")
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	ClassIndex = GemRB.FindTableValue (ClassTable, 5, Class)
	ClassName = GemRB.GetTableRowName (ClassTable, ClassIndex)
	KitIndex = GetKitIndex (actor)
	if KitIndex == 0:
		RowName = ClassName
	else:
		KitTable = GemRB.LoadTable ("kitlist")
		RowName = GemRB.GetTableValue (KitTable, KitIndex, 0)
	Row = GemRB.GetTableRowIndex (DualClassTable, RowName)

	# a lookup table for the DualClassTable columns
	classes = [ "FIGHTER", "CLERIC", "MAGE", "THIEF", "DRUID", "RANGER" ]
	matches = []
	Sum = 0
	for col in range(0, GemRB.GetTableColumnCount (DualClassTable)):
		value = GemRB.GetTableValue (DualClassTable, Row, col)
		Sum += value
		if value == 1:
			matches.append (classes[col])

	# cannot dc if all the columns of the DualClassTable are 0
	if Sum == 0:
		return 1

	# if the only choice for dc is already the same as the actors base class
	# TODO maybe you can't dc a kit to the base class
	if Sum == 1 and ClassName in matches and KitIndex == 0:
		return 1

	AlignmentTable = GemRB.LoadTable ("alignmnt")
	AlignsTable = GemRB.LoadTable ("aligns")
	Alignment = GemRB.GetPlayerStat (actor, IE_ALIGNMENT)
	AlignmentColName = GemRB.FindTableValue (AlignsTable, 3, Alignment)
	AlignmentColName = GemRB.GetTableValue (AlignsTable, AlignmentColName, 4)
	Sum = 0
	for classy in matches:
		Sum += GemRB.GetTableValue (AlignmentTable, classy, AlignmentColName)

	# cannot dc if all the available classes forbid the chars alignment
	if Sum == 0:
		return 1

	return 0
	# TODO add limitations for stats (abdc*.2da)
	# TODO (last) level > 1

def KitInfoWindow():
	global KitInfoWindow

	KitInfoWindow = GemRB.LoadWindow (24)

	#back button (setting first, to be less error prone)
	DoneButton = GemRB.GetControl (KitInfoWindow, 2)
	GemRB.SetText (KitInfoWindow, DoneButton, 11973)
	GemRB.SetEvent (KitInfoWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "KitDonePress")

	#kit or class description
	TextArea = GemRB.GetControl (KitInfoWindow,0)

	pc = GemRB.GameGetSelectedPCSingle ()
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	ClassIndex = GemRB.FindTableValue (ClassTable, 5, Class)
	Multi = GemRB.GetTableValue (ClassTable, ClassIndex, 4)
	Dual = IsDualClassed (pc, 1)

	if Multi and Dual[0] == 0: # true multi class
		text = GemRB.GetTableValue (ClassTable, ClassIndex, 1)
		GemRB.SetText (KitInfoWindow, TextArea, text)
		GemRB.ShowModal (KitInfoWindow, MODAL_SHADOW_GRAY)
		return

	KitTable = GemRB.LoadTable ("kitlist")
	KitIndex = GetKitIndex (pc)

	if Dual[0]: # dual class
		# first (previous) kit or class of the dual class
		if Dual[0] == 1:
			text = GemRB.GetTableValue (KitTable, Dual[1], 3)
		elif Dual[0] == 2:
			text = GemRB.GetTableValue (ClassTable, Dual[1], 1)

		GemRB.SetText (KitInfoWindow, TextArea, text)
		GemRB.TextAreaAppend (KitInfoWindow, TextArea, "\n\n")
		text = GemRB.GetTableValue (ClassTable, Dual[2], 1)

	else: # ordinary class or kit
		if KitIndex:
			text = GemRB.GetTableValue (KitTable, KitIndex, 3)
		else:
			text = GemRB.GetTableValue (ClassTable, ClassIndex, 1)

	GemRB.TextAreaAppend (KitInfoWindow, TextArea, text)

	GemRB.ShowModal (KitInfoWindow, MODAL_SHADOW_GRAY)
	return

def LevelUpWindow():
	global LevelUpWindow

	LevelUpWindow = GemRB.LoadWindow (3)

	InfoButton = GemRB.GetControl (LevelUpWindow, 125)
	GemRB.SetText (LevelUpWindow, InfoButton, 13707)
	GemRB.SetEvent (LevelUpWindow, InfoButton, IE_GUI_BUTTON_ON_PRESS, "LevelUpInfoPress")

	DoneButton = GemRB.GetControl (LevelUpWindow, 0)
	GemRB.SetText (LevelUpWindow, DoneButton, 11962)
	GemRB.SetEvent (LevelUpWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "LevelUpDonePress")
	GemRB.SetButtonFlags (LevelUpWindow, DoneButton, IE_GUI_BUTTON_DEFAULT, OP_OR)

	if False:
		HLAButton = GemRB.GetControl (LevelUpWindow, 126)
		GemRB.SetText (LevelUpWindow, HLAButton, "KUKU")
		GemRB.SetEvent (LevelUpWindow, HLAButton, IE_GUI_BUTTON_ON_PRESS, "LevelUpHLAPress")

	## hide "Character Generation"
	GemRB.DeleteControl (LevelUpWindow, 126)

	## name
	Label = GemRB.GetControl (LevelUpWindow, 90)
	GemRB.SetText (LevelUpWindow, Label, GemRB.GetPlayerName(pc))

	## class
	pc = GemRB.GameGetSelectedPCSingle ()
	Label = GemRB.GetControl (LevelUpWindow, 106)
	GemRB.SetText (LevelUpWindow, Label, GetActorClassTitle (pc))

	GemRB.ShowModal (LevelUpWindow, MODAL_SHADOW_GRAY)
	return

def LevelUpDonePress():
	GemRB.UnloadWindow(LevelUpWindow)
	return

def KitDonePress():
	GemRB.UnloadWindow(KitInfoWindow)
	return

###################################################
# End of file GUIREC.py
