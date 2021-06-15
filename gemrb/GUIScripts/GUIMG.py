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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIMG.py - scripts to control mage spells windows from the GUIMG winpack

###################################################

import GemRB
import GameCheck
import GUICommon
import CommonTables
import GUICommonWindows
import Spellbook
from GUIDefines import *
from ie_stats import *
from ie_spells import LS_MEMO

MageWindow = None
MageSpellInfoWindow = None
MageSpellLevel = 0
MageSpellUnmemorizeWindow = None

# bg2 stuff for handling triggers and contingencies
Sorcerer = None
OtherWindow = None
Exclusions = None
ContCond = None
ContTarg = None
SpellType = None
Level = 1

def ToggleSpellWindow(btn, val):
	global Sorcerer
	# added game check, since although sorcerers have almost no use for their spellbook, there's no other way to quickly check spell descriptions
	pc = GemRB.GameGetSelectedPCSingle ()
	Sorcerer = GameCheck.IsBG2() and Spellbook.HasSorcererBook (pc)
	
	ToggleSpellWindow.Args = (btn, val)
	
	if Sorcerer:
		ToggleSorcererWindow(btn, val)
	else:
		ToggleMageWindow(btn, val)

def InitMageWindow (window):
	global MageWindow
	MageWindow = window

	Button = MageWindow.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MagePrevLevelPress)

	Button = MageWindow.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageNextLevelPress)

	#unknown usage
	Button = MageWindow.GetControl (55)
	if Button:
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	#setup level buttons
	if GameCheck.IsBG2():
		for i in range (9):
			Button = MageWindow.GetControl (56 + i)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RefreshMageLevel)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetVarAssoc ("MageSpellLevel", i)

	# Setup memorized spells buttons
	if not Sorcerer:
		for i in range (12):
			Button = MageWindow.GetControl (3 + i)
			color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 64}
			Button.SetBorder (0,color,0,1)
			Button.SetSprites ("SPELFRAM",0,0,0,0,0)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetAnimation ("")
			Button.SetVarAssoc ("SpellButton", i)

	# Setup book spells buttons
	for i in range (GUICommon.GetGUISpellButtonCount()):
		Button = MageWindow.GetControl (27 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("SpellButton", 100 + i)

	UpdateMageWindow (MageWindow)
	return

def UpdateMageWindow (MageWindow):
	global MageMemorizedSpellList, MageKnownSpellList

	MageMemorizedSpellList = []
	MageKnownSpellList = []

	pc = GemRB.GameGetSelectedPCSingle ()
	spelltype = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, spelltype, level, 1)
	
	CantCast = CommonTables.ClassSkills.GetValue (GUICommon.GetClassRowName (pc), "MAGESPELL") == "*"
	GUICommon.AdjustWindowVisibility (MageWindow, pc, CantCast)

	Label = MageWindow.GetControl (0x10000032)
	if GameCheck.IsBG2():
		GemRB.SetToken ("SPELLLEVEL", str(level + 1))
		Label.SetText (10345)
	else:
		GemRB.SetToken ('LEVEL', str(level + 1))
		Label.SetText (12137)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = MageWindow.GetControl (0x10000035)
	Label.SetText (Name)

	known_cnt = GemRB.GetKnownSpellsCount (pc, spelltype, level)
	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
	true_mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, True)
	if not Sorcerer:
		for i in range (12):
			Button = MageWindow.GetControl (3 + i)

			if i < mem_cnt:
				ms = GemRB.GetMemorizedSpell (pc, spelltype, level, i)
				Button.SetSpellIcon (ms['SpellResRef'], 0)
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
				if ms['Flags']:
					Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellUnmemorizeWindow)
				else:
					Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageUnmemorizeSpell)
				Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenMageSpellInfoWindow)
				MageMemorizedSpellList.append (ms['SpellResRef'])
				Button.EnableBorder (0, ms['Flags'] == 0)
				spell = GemRB.GetSpell (ms['SpellResRef'])
				if not spell:
					print("Missing memorised spell!", ms['SpellResRef'])
					continue
				Button.SetTooltip (spell['SpellName'])
			else:
				if i < max_mem_cnt:
					Button.SetFlags (IE_GUI_BUTTON_NORMAL | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_SET)
				else:
					Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
				Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
				Button.SetTooltip ('')
				Button.EnableBorder (0, 0)
	else:
		label = MageWindow.GetControl (0x10000040)
		if known_cnt:
			# we give sorcerers all charges for all the spells, so some extra math is needed
			label.SetText (GemRB.GetString(61256) + " " + str(true_mem_cnt // known_cnt) + "/" + str(max_mem_cnt))
		else:
			label.SetText ("")
	
	btncount = GUICommon.GetGUISpellButtonCount()
	for i in range (known_cnt):
		Button = MageWindow.GetControl (27 + i)
		Button.SetAnimation ("")
		
		ks = GemRB.GetKnownSpell (pc, spelltype, level, i)
		Button.SetSpellIcon (ks['SpellResRef'], 0)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageMemorizeSpell)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenMageSpellInfoWindow)
		MageKnownSpellList.append (ks['SpellResRef'])
		spell = GemRB.GetSpell (ks['SpellResRef'])
		if not spell:
			print("Missing known spell!", ms['SpellResRef'])
			continue
		Button.SetTooltip (spell['SpellName'])
	
	for i in range (i, btncount):
		Button = MageWindow.GetControl (27 + i)
		Button.SetAnimation ("")
		
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Button.SetTooltip ('')
		Button.EnableBorder (0, 0)

	
	return

def MageSelectionChanged (oldwin):
	global Sorcerer
	# added game check, since although sorcerers have almost no use for their spellbook, there's no other way to quickly check spell descriptions
	
	UpdateMageWindow(oldwin)
	
	pc = GemRB.GameGetSelectedPCSingle ()
	Sorcerer = GameCheck.IsBG2() and Spellbook.HasSorcererBook (pc)

	if Sorcerer:
		OpenSorcererWindow(*ToggleSpellWindow.Args)
	else:
		OpenMageWindow(*ToggleSpellWindow.Args)

ToggleMageWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMG", GUICommonWindows.ToggleWindow, InitMageWindow, MageSelectionChanged)
OpenMageWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMG", GUICommonWindows.OpenWindowOnce, InitMageWindow, MageSelectionChanged)

ToggleSorcererWindow = GUICommonWindows.CreateTopWinLoader(8, "GUIMG", GUICommonWindows.ToggleWindow, InitMageWindow, MageSelectionChanged)
OpenSorcererWindow = GUICommonWindows.CreateTopWinLoader(8, "GUIMG", GUICommonWindows.OpenWindowOnce, InitMageWindow, MageSelectionChanged)

def MagePrevLevelPress ():
	global MageSpellLevel

	if MageSpellLevel > 0:
		MageSpellLevel = MageSpellLevel - 1
		UpdateMageWindow (MageWindow)
	return

def MageNextLevelPress ():
	global MageSpellLevel

	if MageSpellLevel < 8:
		MageSpellLevel = MageSpellLevel + 1
		UpdateMageWindow (MageWindow)
	return

def RefreshMageLevel ():
	global MageSpellLevel

	MageSpellLevel = GemRB.GetVar ("MageSpellLevel")
	UpdateMageWindow (MageWindow)
	return

def OpenMageSpellInfoWindow ():
	global MageSpellInfoWindow

	if MageSpellInfoWindow != None:
		if MageSpellInfoWindow:
			MageSpellInfoWindow.Unload ()
		MageSpellInfoWindow = None
		return

	MageSpellInfoWindow = Window = GemRB.LoadWindow (3)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellInfoWindow)

	#erase
	index = GemRB.GetVar ("SpellButton")
	Button = Window.GetControl (6)
	if Button:
		if index < 100 or Sorcerer:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellRemoveWindow)
			Button.SetText (63668)
	if index < 100:
		ResRef = MageMemorizedSpellList[index]
	else:
		ResRef = MageKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef, 1)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton") - 100
	blend = 1
	if GameCheck.IsBG2():
		blend = 0

	if GemRB.MemorizeSpell (pc, spelltype, level, index):
		UpdateMageWindow (MageWindow)
		GemRB.PlaySound ("GAM_24")
		Button = MageWindow.GetControl(index + 27)
		Button.SetAnimation ("FLASH", 0, blend)
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
		Button = MageWindow.GetControl(mem_cnt + 2)
		Button.SetAnimation ("FLASH", 0, blend)
	return

def CloseMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	if MageSpellUnmemorizeWindow:
		MageSpellUnmemorizeWindow.Unload ()
	MageSpellUnmemorizeWindow = None
	return

def OpenMageSpellRemoveWindow ():
	global MageSpellUnmemorizeWindow

	if GameCheck.IsBG2():
		MageSpellUnmemorizeWindow = GemRB.LoadWindow (101)
	else:
		MageSpellUnmemorizeWindow = GemRB.LoadWindow (5)

	Window = MageSpellUnmemorizeWindow

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (63745)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageRemoveSpell)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseMageSpellUnmemorizeWindow)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenMageSpellUnmemorizeWindow (btn, val):
	global MageSpellUnmemorizeWindow

	if GameCheck.IsBG2():
		MageSpellUnmemorizeWindow = GemRB.LoadWindow (101)
	else:
		MageSpellUnmemorizeWindow = GemRB.LoadWindow (5)
	Window = MageSpellUnmemorizeWindow

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OnMageUnmemorizeSpell(btn, val))
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseMageSpellUnmemorizeWindow)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageUnmemorizeSpell (btn, index):
	if MageSpellUnmemorizeWindow:
		CloseMageSpellUnmemorizeWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	blend = 1
	if GameCheck.IsBG2():
		blend = 0

	if GemRB.UnmemorizeSpell (pc, spelltype, level, index):
		UpdateMageWindow (MageWindow)
		GemRB.PlaySound ("GAM_44")
		Button = MageWindow.GetControl(index + 3)
		Button.SetAnimation ("FLASH", 0, blend)
	return

def OnMageRemoveSpell ():
	CloseMageSpellUnmemorizeWindow()
	OpenMageSpellInfoWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton")-100

	#remove spell from book
	GemRB.RemoveSpell (pc, spelltype, level, index)
	UpdateMageWindow (MageWindow)
	return

def LoadCondition ():
	global ContCond, ContTarg

	Table = GemRB.LoadTable ("contcond")
	CondCount = Table.GetRowCount ()
	ContCond = [0] * CondCount

	for i in range (CondCount):
		#get the condition's name and description
		tuple = (Table.GetValue (i, 0),Table.GetValue (i, 1) )
		ContCond[i] = tuple

	Table = GemRB.LoadTable ("conttarg")
	TargCount = Table.GetRowCount ()
	ContTarg = [0] * TargCount

	for i in range (TargCount):
		#get the condition's name and description
		tuple = (Table.GetValue (i, 0),Table.GetValue (i, 1) )
		ContTarg[i] = tuple
	return

def OpenSequencerWindow ():
	global OtherWindow
	global ContingencyTextArea, TypeButton, OkButton
	global Level, MaxLevel, Target, Count
	global pc
	global ContCond, ContTarg
	global Spell1, Spell2, Spell3, Source

	if ContCond == None:
		LoadCondition()

	Level = 1
	Spell1 = ""
	Spell2 = ""
	Spell3 = ""
	#the target player (who receives the contingency or sequencer)
	pc = GemRB.GetVar("P0")
	ClassName = GUICommon.GetClassRowName (pc)
	#maximum spell level
	MaxLevel = GemRB.GetVar("P1")
	#target 0 - any, 1 - caster only, 2 - sequencer
	#spell count 1-3
	p2 = GemRB.GetVar("P2")
	Source = GemRB.GetSpellCastOn(pc)

	print("Source: ", Source)
	Target = p2>>16
	Count = p2&255
	if Count > 3:
		Count = 3

	#saving the original portrait window
	OtherWindow = Window = GemRB.LoadWindow (6, "GUIMG")

	Title = Window.GetControl (0x0fffffff)

	ContingencyTextArea = Window.GetControl (25)

	CondSelect = Window.GetControl (4)
	CondLabel = Window.GetControl (0x10000000)
	TargSelect = Window.GetControl (6)
	TargLabel = Window.GetControl (0x10000001)
	TypeButton = Window.GetControl (8)

	#no cleric spells available
	if CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL") == "*" and\
			CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL") == "*":
		TypeButton.SetState (IE_GUI_BUTTON_DISABLED)

	if Target == 2:
		CondSelect.SetPos (-1,-1)
		CondLabel.SetPos (-1,-1)
		TargSelect.SetPos (-1,-1)
		TargLabel.SetPos (-1,-1)
	else:
		CondSelect.SetEvent (IE_GUI_TEXTAREA_ON_SELECT, ContingencyHelpCondition)
		CondSelect.SetOptions ([elem[0] for elem in ContCond], "ContCond", 0)

		TargSelect.SetEvent (IE_GUI_TEXTAREA_ON_SELECT, ContingencyHelpTarget)
		if Target:
			TargSelect.SetOptions ([ContTarg[0][0]], "ContTarg", 1)
		else:
			TargSelect.SetOptions ([elem[0] for elem in ContTarg], "ContTarg", 0)

	GemRB.SetVar ("SpellType", 0)
	TypeButton.SetVarAssoc ("SpellType", 1)
	TypeButton.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	TypeButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ContTypePressed)

	Button = Window.GetControl (9)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, LevelIncrease)
	Button = Window.GetControl (10)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, LevelDecrease)

	OkButton = Window.GetControl (27)
	OkButton.SetText (11973)
	OkButton.MakeDefault()
	OkButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = Window.GetControl (29)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	OkButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ContingencyOk)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ContingencyCancel)
	ContTypePressed ()
	Window.ShowModal (MODAL_SHADOW_GRAY)

	# this is here because core runs the selection change event handler too often
	if Target == 2:
		if Count < 3:
			Title.SetText (55374)
			ContingencyTextArea.SetText (60420)
		else:
			Title.SetText (55375)
			ContingencyTextArea.SetText (55372)
	else:
		if Count < 3:
			Title.SetText (11941)
		else:
			Title.SetText (55376)
		ContingencyTextArea.SetText (55373)

	return

def UpdateSpellList ():
	Window = OtherWindow

	if SpellType:
		Type = IE_SPELL_TYPE_PRIEST
	else:
		Type = IE_SPELL_TYPE_WIZARD

	Label = Window.GetControl (0x1000001d)
	GemRB.SetToken ('LEVEL', str(Level))
	Label.SetText (12137)

	BuildSpellList(pc, Type, Level-1)

	names = list(SpellList.keys())
	names.sort()

	cnt = len(names)
	j = 0
	for i in range(11, 22):
		Button = Window.GetControl (i)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM,OP_OR)
		if j<cnt:
			Button.SetSpellIcon (names[j], 1)
			Button.SetText( str(SpellList[names[j]]) )
			Button.SetVarAssoc("PickedSpell", j)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ContingencyHelpSpell)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetSpellIcon("")
			Button.SetText("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		j = j+1

	Button = Window.GetControl (22)
	Button.SetSpellIcon(Spell1, 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteSpell1)
	Button = Window.GetControl (23)
	Button.SetSpellIcon(Spell2, 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteSpell2)
	Button = Window.GetControl (24)
	Button.SetSpellIcon(Spell3, 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteSpell3)

	if not Spell1:
		OkButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		OkButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def BuildSpellList (pc, spelltype, level):
	global SpellList

	import Spellbook

	if not Exclusions:
		LoadExclusions()

	SpellList = {}
	memoedSpells = Spellbook.GetMemorizedSpells (pc, spelltype, level)

	# are we a sorcerer-style?
	SorcererBook = Spellbook.HasSorcererBook (pc)

	names = []
	for s in memoedSpells:
		names.append(s["SpellResRef"])

	if SorcererBook:
		dec = 0
		for selected in Spell1, Spell2, Spell3:
			if selected in names:
				dec += 1
		# decrease count for all spells
		if dec > 0:
			for ms in memoedSpells:
				ms["MemoCount"] -= dec
	else:
		for s in memoedSpells:
			sref = s["SpellResRef"]
			for selected in Spell1, Spell2, Spell3:
				if sref == selected:
					s["MemoCount"] -= 1

	for s in memoedSpells:
		if s["MemoCount"] < 1:
			continue

		spell = s["SpellResRef"]
		if spell in Exclusions[level]:
			continue
		SpellList[spell] = s["MemoCount"]
	return

def ContTypePressed ():
	global SpellType

	Label = OtherWindow.GetControl (0x10000002)
	SpellType = GemRB.GetVar ("SpellType")

	if SpellType:
		Label.SetText (54352)
		TypeButton.SetText (18039)
	else:
		Label.SetText (21836)
		TypeButton.SetText (7204)
	UpdateSpellList()
	return

def ContingencyOk ():
	global OtherWindow

	#set the storage
	if Target == 2:
		GemRB.ApplyEffect (pc, "Sequencer:Store", 0, 0, Spell1, Spell2, Spell3, Source)
	else:
		GemRB.ApplyEffect (pc, "CastSpellOnCondition", 0, GemRB.GetVar ("ContCond"), Spell1, Spell2, Spell3, Source)
	#set the innate
	if GemRB.LearnSpell (pc, Source+"d", LS_MEMO):
		GemRB.Log (LOG_ERROR, "ContingencyOk", "Failed to learn sequencer/contingency!")
	OtherWindow.Unload()
	return

def ContingencyCancel ():
	global OtherWindow

	GemRB.SetPlayerStat (pc, IE_IDENTIFYMODE, 0)
	OtherWindow.Unload()
	return

def ContingencyHelpSpell ():
	global Spell1, Spell2, Spell3

	names = list(SpellList.keys())
	names.sort()
	i = GemRB.GetVar("PickedSpell")
	spell = names[i]

	if Spell1=="" and Count>0:
		Spell1 = spell
	elif Spell2=="" and Count>1:
		Spell2 = spell
	elif Spell3=="" and Count>2:
		Spell3 = spell

	spl = GemRB.GetSpell(spell)
	ContingencyTextArea.SetText (spl["SpellDesc"])
	UpdateSpellList ()
	return

def DeleteSpell1 ():
	global Spell1, Spell2, Spell3

	Spell1 = Spell2
	Spell2 = Spell3
	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def DeleteSpell2 ():
	global Spell2, Spell3

	Spell2 = Spell3
	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def DeleteSpell3 ():
	global Spell3

	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def ContingencyHelpCondition ():
	i = GemRB.GetVar ("ContCond")
	ContingencyTextArea.SetText (ContCond[i][1])
	return

def ContingencyHelpTarget ():
	i = GemRB.GetVar ("ContTarg")
	ContingencyTextArea.SetText (ContTarg[i][1])
	return

def LoadExclusions():
	global Exclusions

	ExclusionTable = GemRB.LoadTable ("contingx")
	Columns = ExclusionTable.GetColumnCount ()
	Rows = ExclusionTable.GetRowCount ()
	Exclusions = []
	for i in range (Columns):
		Exclusions.append ([])
		for j in range (Rows):
			spell = ExclusionTable.GetValue (j, i, GTV_STR)
			if spell[0]=="*":
				break
			Exclusions[i].append (spell.lower())

	return

def LevelIncrease():
	global Level

	if Level<MaxLevel:
		Level = Level+1
	UpdateSpellList()
	return

def LevelDecrease():
	global Level

	if Level>1:
		Level = Level-1
	UpdateSpellList()
	return

###################################################
# End of file GUIMG.py
