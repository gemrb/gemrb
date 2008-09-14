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
# $Id$


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack

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
OldOptionsWindow = None
OptionsWindow = None

###################################################
def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow
	global OldOptionsWindow

	if CloseOtherWindow (OpenRecordsWindow):
		if InformationWindow: OpenInformationWindow ()

                
		GemRB.UnloadWindow (RecordsWindow)
		GemRB.UnloadWindow (OptionsWindow)
		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	
	GemRB.LoadWindowPack ("GUIREC")
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow)
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenRecordsWindow")
	GemRB.SetWindowFrame (OptionsWindow)

	# dual class
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 7174)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DualClassWindow")

	# levelup
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetText (Window, Button, 7175)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LevelupWindow")

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

## 	# kit info
## 	Button = GemRB.GetControl (Window, 52)
## 	GemRB.SetText (Window, Button, 61265)
## 	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "KitInfoWindow")

	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()

	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (GUICommonWindows.PortraitWindow, 1)
	return

def GetNextLevelExp (Level, Class):
	NextLevelTable = GemRB.LoadTable ("XPLEVEL")
	Row = GemRB.GetTableRowIndex (NextLevelTable, Class)
	if Level < GemRB.GetTableColumnCount (NextLevelTable, Row):
		return str(GemRB.GetTableValue (NextLevelTable, Row, Level) )

	return 0;

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

	# name
	Label = GemRB.GetControl (Window, 0x1000000e)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture (Window, Button, GemRB.GetPlayerPortrait (pc,0), "NOPORTLG")

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

	text = GemRB.GetTableValue (Table, GemRB.FindTableValue( Table, 3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT) ), 0)
	GemRB.UnloadTable (Table)
	Label = GemRB.GetControl (Window, 0x10000010)
	GemRB.SetText (Window, Label, text)

	Label = GemRB.GetControl (Window, 0x10000011)
	if GemRB.GetPlayerStat (pc, IE_SEX) ==  1:
		GemRB.SetText (Window, Label, 7198)
	else:
		GemRB.SetText (Window, Label, 7199)

	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = GemRB.GetControl (Window, 45)
	GemRB.SetText (Window, Text, stats_overview)
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
	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GA = lambda s, col, pc=pc: GemRB.GetAbilityBonus (s, col, GS (s) )

	stats = []
	# class levels
	# 16480 <CLASS>: Level <LEVEL>
	# Experience: <EXPERIENCE>
	# Next Level: <NEXTLEVEL>

	#collecting tokens for stat overview
	ClassTitle = GemRB.GetString (GetActorClassTitle (pc) )
	GemRB.SetToken("CLASS", ClassTitle)
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	Class = GemRB.FindTableValue (ClassTable, 5, Class)
	Multi = GemRB.GetTableValue (ClassTable, Class, 4)
	Class = GemRB.GetTableRowName (ClassTable, Class)
	if Multi:
		Levels = [IE_LEVEL, IE_LEVEL2, IE_LEVEL3]
		Classes = [0,0,0]
		MultiCount = 0
		stats.append ( (19721,1,'c') )
		Mask = 1
		for i in range (16):
			if Multi&Mask:
				Classes[MultiCount] = GemRB.FindTableValue (ClassTable, 5, i+1)
				MultiCount += 1
			Mask += Mask

		for i in range (MultiCount):
			#todo get classtitle for this class
			Class = Classes[i]
			ClassTitle = GemRB.GetString(GemRB.GetTableValue (ClassTable, Class, 2))
			GemRB.SetToken("CLASS", ClassTitle)
			Class = GemRB.GetTableRowName (ClassTable, i)
			Level = GemRB.GetPlayerStat (pc, Levels[i])
			GemRB.SetToken("LEVEL", str (Level) )
			GemRB.SetToken("NEXTLEVEL", GetNextLevelExp (Level, Class) )
			GemRB.SetToken("EXPERIENCE", str (GemRB.GetPlayerStat (pc, IE_XP)/MultiCount ) )
			#resolve string immediately
			stats.append ( (GemRB.GetString(16480),"",'b') )
			stats.append (None)

	else:
		Level = GemRB.GetPlayerStat (pc, IE_LEVEL)
		GemRB.SetToken("LEVEL", str (Level) )
		GemRB.SetToken("NEXTLEVEL", GetNextLevelExp (Level, Class) )
		GemRB.SetToken("EXPERIENCE", str (GemRB.GetPlayerStat (pc, IE_XP) ) )
		stats.append ( (16480,1,'c') )

	stats.append (None)
	GemRB.UnloadTable (ClassTable)

	#Current State
	StateTable = GemRB.LoadTable ("statdesc")
	effects = GemRB.GetPlayerStates (pc)
	for c in effects:
		tmp = GemRB.GetTableValue (StateTable, ord(c)-66, 0)
		stats.append ( (tmp,c,'a') )
	stats.append (None)

	#proficiencies
	stats.append ( (8442,1,'c') )
	stats.append ( (9457, GS (IE_THAC0), '') )
	tmp = GS (IE_NUMBEROFATTACKS)
	if (tmp&1):
		tmp2 = str(tmp/2) + chr(188)
	else:
		tmp2 = str(tmp/2)
	stats.append ( (9458, tmp2, '') )
	stats.append ( (9459, GS (IE_LORE), '') )
	stats.append ( (19224, GS (IE_MAGICDAMAGERESISTANCE), '') )
	#reputation
	reptxt = GetReputation (GemRB.GameGetReputation()/10)
	stats.append ( (9465, reptxt, '') )
	#script
	aiscript = GemRB.GetPlayerScript (pc )
	stats.append ( (2078, aiscript, '') )
	stats.append (None)

	# 17379 Saving throws
	stats.append (17379)
	#   17380 Paralyze/Poison/Death
	stats.append ((17380, GS (IE_SAVEVSDEATH), ''))
	#   17381 Rod/Staff/Wand
	stats.append ((17381, GS (IE_SAVEVSWANDS), ''))
	#   17382 Petrify/Polymorph
	stats.append ((17382, GS (IE_SAVEVSPOLY), ''))
	#   17383 Breath Weapon
	stats.append ((17383, GS (IE_SAVEVSBREATH), ''))
	#   17384 Spells
	stats.append ((17384, GS (IE_SAVEVSSPELL), ''))
	stats.append (None)

	# 9466 Weapon Proficiencies
	stats.append (9466)
	table = GemRB.LoadTable("weapprof")
	RowCount = GemRB.GetTableRowCount (table)
	for i in range(RowCount):
		text = GemRB.GetTableValue (table, i, 1)
		stat = GemRB.GetTableValue (table, i, 0)
		stats.append ( (text, GS(stat), '+') )
	stats.append (None)

	# 11766 AC Bonuses
	stats.append (11766)
	#   11767 AC vs. Missile
	stats.append ((11767, GS (IE_ACMISSILEMOD), ''))
	#   11768 AC vs. Piercing
	stats.append ((11768, GS (IE_ACSLASHINGMOD), ''))
	#   11769 AC vs. Crushing
	stats.append ((11769, GS (IE_ACPIERCINGMOD), ''))
	#   11770 AC vs. Slashing
	stats.append ((11770, GS (IE_ACCRUSHINGMOD), ''))
	stats.append (None)

	# 10315 Ability Bonuses
	stats.append (10315)
	#   10332 To Hit
	stats.append ((10332, GA (IE_STR,0), '0' ))
	#   10336 Damage
	stats.append ((10336, GA (IE_STR,1), '0' ))
	#   10337 Open Doors
	stats.append ((10337, GA (IE_STR,2), '0' ))
	#   10338 Weight Allowance
	stats.append ((10338, GA (IE_STR,3), '0' ))
	#   10339 Armor Class
	stats.append ((10339, GA (IE_DEX,0), '0' ))
	#   10340 Missile Adjustment
	stats.append ((10340, GA (IE_DEX,1), '0' ))
	#   10341 Reaction Adjustment
	stats.append ((10341, GA (IE_DEX,2), '0' ))
	#   10342 CON HP Bonus/Level
	stats.append ((10342, GA (IE_CON,0), '0' ))
	#   10343 Chance To Learn spell
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, 0, 0)>0:
		stats.append ((10343, GA (IE_INT,0), '%' ))
	#   10347 Reaction
	stats.append ((10347, GA (IE_CHR,0), '0' ))

	#   10344 Bonus Priest Spell
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

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			if type == '+':
				res.append ("[capital=0]"+GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'x':
				res.append ("[capital=0]"+GemRB.GetString (strref)+': x' + str (val) )
			elif type == 'a':
				res.append ("[capital=2]"+val+" "+GemRB.GetString (strref))
			elif type == 'b':
				res.append ("[capital=0]"+strref+": "+str(val) )
			elif type == 'c':
				res.append ("[capital=0]"+GemRB.GetString (strref))
			elif type == '0':
				res.append (GemRB.GetString (strref) + ': ' + str (val) )
			else:
				res.append ("[capital=0]"+GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append ( GemRB.GetString (s) )
				lines = 0
			else:
				if lines:
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

		return

	InformationWindow = Window = GemRB.LoadWindow (4)

	# Biography
	Button = GemRB.GetControl (Window, 26)
	GemRB.SetText (Window, Button, 18003)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 24)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseInformationWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseInformationWindow ():
        global InformationWindow

        GemRB.UnloadWindow (InformationWindow)
	InformationWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (RecordsWindow, 1)
	GemRB.SetVisible (GUICommonWindows.PortraitWindow, 1)
	return

def OpenBiographyWindow ():
	global BiographyWindow

	if BiographyWindow != None:
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)
	GemRB.SetVar ("FloatWindow", BiographyWindow)

	TextArea = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, TextArea, 39424)

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseBiographyWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseBiographyWindow ():
        global BiographyWindow

        GemRB.UnloadWindow (BiographyWindow)
	BiographyWindow = None
	GemRB.SetVisible (InformationWindow, 1)
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
	return

def ExportCancelPress():
	GemRB.UnloadWindow(ExportWindow)
	return

def ExportEditChanged():
	ExportFileName = GemRB.QueryText(ExportWindow, NameField)
	if ExportFileName == "":
		GemRB.SetButtonState(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState(ExportWindow, ExportDoneButton, IE_GUI_BUTTON_ENABLED)
	return

###################################################
# End of file GUIREC.py
