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

ActiveSpellBooks = []
BookCount = 0
BookTopIndex = 0
BookNames = (1083,1079,1080,1078,1077,32,1081,39722)

def InitSpellBookWindow (Window):
	global SpellBookWindow

	SpellBookWindow = Window

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
		color = {'r' : 0, 'g' : 0, 'b' : 0, 'a' : 160}
		Button.SetBorder (0,color,0,1)
		#Button.SetBAM ("SPELFRAM",0,0,0)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS | IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	# Setup book spells buttons
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetFlags (IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("SpellIndex", i)

	return

def SelectedNewPlayer (Window):
	global ActiveSpellBooks
	global BookTopIndex, BookCount

	pc = GemRB.GameGetSelectedPCSingle ()
	ActiveSpellBooks=[]

	for i in range(8):
		if GemRB.GetMemorizableSpellsCount (pc, i, 0) > 0:
			ActiveSpellBooks+=[i]

	BookCount = len(ActiveSpellBooks)
	BookTopIndex = 0
	if GemRB.GetVar ("SelectedBook") == 0:
		if len (ActiveSpellBooks):
			SelectedBook = ActiveSpellBooks[0]
		else:
			SelectedBook = 0
		GemRB.SetVar ("SelectedBook", SelectedBook)
	UpdateSpellBook ()
	return

ToggleSpellBookWindow = GUICommonWindows.CreateTopWinLoader(2, "GUISPL", GUICommonWindows.ToggleWindow, InitSpellBookWindow, SelectedNewPlayer)
OpenSpellBookWindow = GUICommonWindows.CreateTopWinLoader(2, "GUISPL", GUICommonWindows.OpenWindowOnce, InitSpellBookWindow, SelectedNewPlayer)

def ResetScrollBar ():
	pc = GemRB.GameGetSelectedPCSingle ()
	ScrollBar = SpellBookWindow.GetControl (54)
	GemRB.SetVar ("SpellTopIndex",0)
	ScrollBar.SetVarAssoc ("SpellTopIndex", len(KnownSpellList))
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	SpellBookWindow.SetEventProxy(ScrollBar)

def ScrollBarPress ():
	UpdateSpellBookWindow ()

def UpdateSpellBook ():
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
			BookType = ActiveSpellBooks[BookTopIndex + i]
			Button.SetText (BookNames[BookType])
			Button.SetVarAssoc ("SelectedBook", BookType)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateSpellBook)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")

	BookType = GemRB.GetVar ("SelectedBook")
	level = SpellBookSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, BookType, level)
	sorcerer_style = (BookType == IE_IWD2_SPELL_BARD) or (BookType == IE_IWD2_SPELL_SORCERER)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0xfffffff)
	Label.SetText (Name)

	Button = Window.GetControl (1)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc, 0)["Sprite"])
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	mem_cnt = GetMemorizedSpellsCount () # count of unique memorized spells
	true_mem_cnt = GemRB.GetMemorizedSpellsCount (pc, BookType, level, True)
	for i in range (24):
		Button = Window.GetControl (6 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x1000003f+i)
		# actually, it doesn't display any memorized spells for sorcerer-style spellbooks
		if i < len(MemorizedSpellList) and not sorcerer_style:
			ms = MemorizedSpellList[i]
			spell = ms['SpellResRef']
			Button.SetSpellIcon (spell)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
			Button.SetTooltip (ms['SpellName'])
			Button.SetVarAssoc ("SpellButton", i)
			# since spells are stacked, we need to check first whether to unmemorize (deplete) or remove (already depleted)
			if ms['MemoCount'] < ms['KnownCount']:
				# already depleted, just remove
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OnSpellBookUnmemorizeSpell (ms['MemoCount']))
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
	# and they display just the current and total number of memorizations per level
	Label = Window.GetControl (0x10000004)
	if sorcerer_style:
		available = "0"
		if known_cnt:
			available = str(true_mem_cnt / known_cnt)
		Label.SetText (available+"/"+str(max_mem_cnt))
	else:
		# reset mem_cnt to take into account stacks
		mem_cnt = GetMemorizedSpellsCount (True)
		Label.SetText (str(max_mem_cnt-mem_cnt))

	#if actor is uncontrollable, make this grayed
	CantCast = GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	return

def GetMemorizedSpellsCount (total=False):
	'''count the real number of spells in MemorizedSpellList'''
	# can't use len here, since there could be more than one memorization (a "stack")
	# put the counts into a list and sum it, but be careful about depleted spells
	# eg. fully depleted should still count as 1 and each partial too
	count = 'MemoCount' # non-depleted only
	if total:
		count = 'KnownCount' # all

	counts = map (lambda x: max(x[count], 1), MemorizedSpellList)
	return sum(counts)

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
	SelectedBook = GemRB.GetVar ("SelectedBook")
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

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.MakeEscape()
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellInfoWindow)

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	BookType = GemRB.GetVar ("SelectedBook")
	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ms = GemRB.GetMemorizedSpell (pc, BookType, level, index)
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

# FIXME: for now don't do anything if paused, since we'd get stuck with the first frame
# EH, doesn't work on the known spell buttons?
def FlashOverButton (ControlID):
	pause = GemRB.GamePause (10, 1) # quietly get the pause state
	if pause == 1: # paused
		return
	Button = SpellBookWindow.GetControl(ControlID)
	Button.SetAnimation ("FLASH")
	# TODO: wait until it is done; the two flashes for memorizing were not shown in parallel

def OnSpellBookMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	BookType = GemRB.GetVar ("SelectedBook")
	Window = SpellBookWindow

	index = GemRB.GetVar ("SpellButton") - 100
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	if GemRB.MemorizeSpell (pc, BookType, level, index):
		RefreshSpellBookLevel () # calls also UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_24")
		FlashOverButton (index - SpellTopIndex + 30) # FIXME: wrong button if the spell will be stacked
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, BookType, level, False)
		FlashOverButton (mem_cnt + 5)
	return

def OpenSpellBookSpellRemoveWindow ():
	global SpellBookSpellUnmemorizeWindow

	if SpellBookSpellUnmemorizeWindow != None:
		if SpellBookSpellUnmemorizeWindow:
			SpellBookSpellUnmemorizeWindow.Unload ()
		SpellBookSpellUnmemorizeWindow = None
		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnSpellBookRemoveSpell)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellRemoveWindow)
	Button.MakeEscape()

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
	BookType = GemRB.GetVar ("SelectedBook")

	# remove spell from memory
	if GemRB.UnmemorizeSpell (pc, BookType, level, index, onlydepleted):
		UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_44")
		FlashOverButton (index + 6)
	else:
		print "Spell unmemorization failed, huh?", pc, BookType, level, index, onlydepleted

	RefreshSpellBookLevel (False)
	UpdateSpellBookWindow ()

###################################################
# End of file GUISPL.py
