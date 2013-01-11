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


# GUISPL.py - scripts to control spells windows from GUISPL winpack

###################################################

import GemRB
import GUICommon
import GUICommonWindows
import Spellbook
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_CAST

SpellBookWindow = None
SpellBookSpellInfoWindow = None
SpellBookSpellLevel = 0
SpellBookSpellUnmemorizeWindow = None
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None
ActiveSpellBooks = []
BookCount = 0
BookTopIndex = 0
SelectedBook = 0
BookNames = (1083,1079,1080,1078,1077,32,1081,39722)

def OpenSpellBookWindow ():
	global SpellBookWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenSpellBookWindow):
		if SpellBookWindow:
			SpellBookWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		SpellBookWindow = None
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

	GemRB.LoadWindowPack ("GUISPL", 800, 600)
	SpellBookWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", SpellBookWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenSpellBookWindow)
	Window.SetFrame ()

	Button = Window.GetControl (92)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SpellBookPrevPress)

	Button = Window.GetControl (93)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SpellBookNextPress)

	#setup level buttons
	for i in range (9):
		Button = Window.GetControl (55 + i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RefreshSpellBookLevel)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("SpellBookSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (24):
		Button = Window.GetControl (6 + i)
		Button.SetBorder (0,0,0,0,0,0,0,0,160,0,1)
		#Button.SetBAM ("SPELFRAM",0,0,0)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	# Setup book spells buttons
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetFlags (IE_GUI_BUTTON_PLAYONCE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("SpellIndex", i)

	GUICommonWindows.SetSelectionChangeHandler (SelectedNewPlayer)
	SelectedNewPlayer ()
	return

def SelectedNewPlayer ():
	global ActiveSpellBooks
	global BookTopIndex, BookCount, SelectedBook
	
	Window = SpellBookWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	ActiveSpellBooks=[]
	
	for i in range(8):
		if GemRB.GetKnownSpellsCount(pc, i)>0:
			ActiveSpellBooks+=[i]
	BookCount = len(ActiveSpellBooks)
	BookTopIndex = 0
	if len (ActiveSpellBooks):
		SelectedBook = ActiveSpellBooks[0]
	else:
		SelectedBook = 0
	GemRB.SetVar ("SelectedBook",SelectedBook)
	UpdateSpellBook ()
	return

def ResetScrollBar ():
	pc = GemRB.GameGetSelectedPCSingle ()
	ScrollBar = SpellBookWindow.GetControl (54)
	GemRB.SetVar ("SpellTopIndex",0)
	ScrollBar.SetVarAssoc ("SpellTopIndex", len(KnownSpellList))
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	ScrollBar.SetDefaultScrollBar ()

def ScrollBarPress ():
	UpdateSpellBookWindow ()

def UpdateSpellBook ():
	global SelectedBook

	SelectedBook = GemRB.GetVar ("SelectedBook")
	RefreshSpellBookLevel (False) # set up all the spell lists first
	ResetScrollBar ()
	UpdateSpellBookWindow ()

def UpdateSpellBookWindow ():
	global SpellBookSpellLevel

	Window = SpellBookWindow
	pc = GemRB.GameGetSelectedPCSingle ()

	for i in range(4):
		Button = Window.GetControl(88+i)
		
		if len(ActiveSpellBooks)>BookTopIndex+i:
			type = ActiveSpellBooks[BookTopIndex+i]
			Button.SetText (BookNames[type])
			Button.SetVarAssoc ("SelectedBook", type)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateSpellBook)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")

	SelectedBook = GemRB.GetVar("SelectedBook")
	type = SelectedBook
	level = SpellBookSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)
	sorcerer_style = (type == IE_IWD2_SPELL_BARD) or (type == IE_IWD2_SPELL_SORCEROR)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0xfffffff)
	Label.SetText (Name)

	Button = Window.GetControl (1)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	mem_cnt = len (MemorizedSpellList)
	true_mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level, True)
	for i in range (24):
		Button = Window.GetControl (6 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x1000003f+i)
		# actually, it doesn't display any memorized spells for sorcerer-style spellbooks
		if i < mem_cnt and not sorcerer_style:
			ms = MemorizedSpellList[i]
			spell = ms['SpellResRef']
			Button.SetSpellIcon (spell)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE, OP_OR)
			Button.SetTooltip (ms['SpellName'])
			Button.SetVarAssoc ("SpellButton", i)
			# since spells are stacked, we need to check first whether to unmemorize (deplete) or remove (already depeleted)
			if ms['MemoCount'] < ms['KnownCount']:
				# already depleted, just remove
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OnSpellBookUnmemorizeSpell(ms['MemoCount']))
			else:
				# deplete and remove
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellRemoveWindow)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenSpellBookSpellInfoWindow)
			tmp = str(ms['MemoCount'])+"/"+str(ms['KnownCount'])
			Label.SetText (tmp)
		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.EnableBorder (0, 0)
			Label.SetText ('')

	known_cnt = len (KnownSpellList)
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x10000025+i)
		if i+SpellTopIndex < known_cnt:
			ks = KnownSpellList[i+SpellTopIndex]
			spell = ks['SpellResRef']
			Button.SetSpellIcon (spell)
			if not sorcerer_style:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnSpellBookMemorizeSpell)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenSpellBookSpellInfoWindow)
			Label.SetText (ks['SpellName'])
			Button.SetVarAssoc ("SpellButton", 100 + i + SpellTopIndex)
		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.EnableBorder (0, 0)
			Label.SetText ('')

	# number of available spell slots
	# sorcerer-style books are different, since max_mem_cnt holds the total level capacity, not the slot count
	# and they display just the current and total number of memorizazions per level
	Label = Window.GetControl (0x10000004)
	if sorcerer_style:
		Label.SetText (str(true_mem_cnt)+"/"+str(max_mem_cnt))
	else:
		Label.SetText (str(max_mem_cnt-mem_cnt))

	#if actor is uncontrollable, make this grayed
	CantCast = GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	return

def SpellBookPrevPress ():
	global BookTopIndex

	BookTopIndex = GemRB.GetVar ("BookTopIndex")
	if BookTopIndex>0:
		BookTopIndex -= 1
	GemRB.SetVar("BookTopIndex", BookTopIndex)
	UpdateSpellBook ()
	return

def SpellBookNextPress ():
	global BookTopIndex

	BookTopIndex = GemRB.GetVar ("BookTopIndex")
	if BookTopIndex<BookCount-4:
		BookTopIndex += 1
	GemRB.SetVar("BookTopIndex", BookTopIndex)
	UpdateSpellBook ()
	return

def RefreshSpellBookLevel (update=True):
	global SpellBookSpellLevel, KnownSpellList, MemorizedSpellList

	SpellBookSpellLevel = GemRB.GetVar ("SpellBookSpellLevel")
	pc = GemRB.GameGetSelectedPCSingle ()
	KnownSpellList = Spellbook.GetKnownSpellsLevel (pc, SelectedBook, SpellBookSpellLevel)
	MemorizedSpellList = Spellbook.GetMemorizedSpells (pc, SelectedBook, SpellBookSpellLevel)
	if update:
		UpdateSpellBookWindow ()
	return

def OpenSpellBookSpellInfoWindow ():
	global SpellBookSpellInfoWindow

	if SpellBookSpellInfoWindow != None:
		SpellBookSpellInfoWindow.Unload ()
		SpellBookSpellInfoWindow = None

		return

	SpellBookSpellInfoWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", SpellBookSpellInfoWindow.ID)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellInfoWindow)

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = SelectedBook
	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ms = GemRB.GetMemorizedSpell (pc, type, level, index)
		ResRef = ms['SpellResRef']
	else:
		ResRef = KnownSpellList[index - 100]["SpellResRef"]
	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return


def OnSpellBookMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = SelectedBook
	Window = SpellBookWindow
	
	index = GemRB.GetVar ("SpellButton") - 100
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	if GemRB.MemorizeSpell (pc, type, level, index):
		RefreshSpellBookLevel () # calls also UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_24")
		Button = Window.GetControl(index - SpellTopIndex + 30) # FIXME: wrong button if the spell will be stacked
		Button.SetAnimation ("FLASH")
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level, False)
		Button = Window.GetControl(mem_cnt + 5)
		Button.SetAnimation ("FLASH")
	return

def OpenSpellBookSpellRemoveWindow ():
	global SpellBookSpellUnmemorizeWindow

	if SpellBookSpellUnmemorizeWindow != None:
		if SpellBookSpellUnmemorizeWindow:
			SpellBookSpellUnmemorizeWindow.Unload ()
		SpellBookSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", SpellBookSpellUnmemorizeWindow.ID)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnSpellBookRemoveSpell)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellRemoveWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

# since we can have semidepleted stacks, first make sure we unmemorize the depleted ones, only then unmemorize
def OnSpellBookUnmemorizeSpell (mem_cnt):
	if mem_cnt == 0:
		# everything is depleted, so just remove
		OnSpellBookRemoveSpell ()
		return

	# remove one depleted spell without touching the still memorized
	# we'd need a different spell index, but it's all handled in the core
	index = GemRB.GetVar ("SpellButton")
	UnmemoSpell (index, True)

	return

# not like removing spells in bg2, where you could delete known spells from the spellbook!
def OnSpellBookRemoveSpell ():
	if SpellBookSpellUnmemorizeWindow:
		OpenSpellBookSpellRemoveWindow ()

	index = GemRB.GetVar ("SpellButton")
	UnmemoSpell (index)

	return

def UnmemoSpell (index, onlydepleted=False):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = SelectedBook

	# remove spell from memory
	if GemRB.UnmemorizeSpell (pc, type, level, index, onlydepleted):
		UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_44")
		Button = SpellBookWindow.GetControl(index + 6)
		Button.SetAnimation ("FLASH")
	else:
		print "Spell unmemorization failed, huh?", pc, type, level, index, onlydepleted

	RefreshSpellBookLevel (False)
	UpdateSpellBookWindow ()

###################################################
# End of file GUISPL.py
