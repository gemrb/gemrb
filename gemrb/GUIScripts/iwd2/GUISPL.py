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
SpellTopIndex = 0
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
	global BookTopIndex, SpellTopIndex, BookCount, SelectedBook
	
	Window = SpellBookWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	ActiveSpellBooks=[]
	
	for i in range(8):
		if GemRB.GetKnownSpellsCount(pc, i)>0:
			ActiveSpellBooks+=[i]
	BookCount = len(ActiveSpellBooks)
	BookTopIndex = 0
	SpellTopIndex = 0
	if len (ActiveSpellBooks):
		SelectedBook = ActiveSpellBooks[0]
	else:
		SelectedBook = 0
		
	ScrollBar = Window.GetControl (54)
	GemRB.SetVar ("SpellTopIndex",0)
	ScrollBar.SetVarAssoc ("SpellTopIndex", len(Spellbook.GetKnownSpellsLevel (pc, SelectedBook, SpellBookSpellLevel))) # make global, move to Update
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	ScrollBar.SetDefaultScrollBar ()
	GemRB.SetVar ("SelectedBook",SelectedBook)
	UpdateSpellBookWindow ()
	return

def ScrollBarPress ():
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
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateSpellBookWindow)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")

	SelectedBook = GemRB.GetVar("SelectedBook")
	type = SelectedBook
	level = SpellBookSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0xfffffff)
	Label.SetText (Name)

	Button = Window.GetControl (1)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))

	Spells = Spellbook.GetMemorizedSpells (pc, type, level)
	mem_cnt = len (Spells)
	for i in range (24):
		Button = Window.GetControl (6 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x1000003f+i)
		if i < mem_cnt:
			ms = Spells[i]
			spell = ms['SpellResRef']
			Button.SetSpellIcon (spell)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE, OP_OR)
			Button.SetTooltip (ms['SpellName'])
			Button.SetVarAssoc ("SpellButton", i)
			tmp = str(ms['MemoCount'])+"/"+str(ms['KnownCount'])
			Label.SetText (tmp)
		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.EnableBorder (0, 0)
			Label.SetText ('')

	Spells = Spellbook.GetKnownSpellsLevel (pc, type, level)
	known_cnt = len (Spells)
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x10000025+i)
		if i+SpellTopIndex < known_cnt:
			ks = Spells[i+SpellTopIndex]
			spell = ks['SpellResRef']
			Button.SetSpellIcon (spell)
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
	Label = Window.GetControl (0x10000004)
	if GemRB.GetPlayerStat (pc, IE_LEVELSORCEROR) or GemRB.GetPlayerStat (pc, IE_LEVELBARD):
		Label.SetText (str(0)) # known_cnt-mem_cnt, but it is always 0
	else:
		Label.SetText (str(max_mem_cnt-mem_cnt))

	#if actor is uncontrollable, make this grayed
	CantCast = GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	return

#TODO: spell type selector
def SpellBookPrevPress ():
	global BookTopIndex, SpellTopIndex

	BookTopIndex = GemRB.GetVar ("BookTopIndex")
	if BookTopIndex>0:
		BookTopIndex -= 1
	SpellTopIndex = 0
	GemRB.SetVar("BookTopIndex", BookTopIndex)
	GemRB.SetVar("SpellTopIndex", SpellTopIndex)
	UpdateSpellBookWindow ()
	return

#TODO: spell type selector
def SpellBookNextPress ():
	global BookTopIndex, SpellTopIndex

	BookTopIndex = GemRB.GetVar ("BookTopIndex")
	if BookTopIndex<BookCount-4:
		BookTopIndex += 1
	SpellTopIndex = 0
	GemRB.SetVar("BookTopIndex", BookTopIndex)
	GemRB.SetVar("SpellTopIndex", SpellTopIndex)
	UpdateSpellBookWindow ()
	return

def RefreshSpellBookLevel ():
	global SpellBookSpellLevel

	SpellBookSpellLevel = GemRB.GetVar ("SpellBookSpellLevel")
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
		# FIXME: ugly, make the list global again like SpellBookKnownSpellList of yore
		ResRef = Spellbook.GetKnownSpellsLevel (pc, type, level)[index - 100]["SpellResRef"]
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

	if GemRB.MemorizeSpell (pc, type, level, index + TopIndex):
		UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_24")
		Button = Window.GetControl(index + 30)
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
	TextArea.SetText (63745)

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

def OpenSpellBookSpellUnmemorizeWindow ():
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnSpellBookUnmemorizeSpell)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSpellBookSpellUnmemorizeWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnSpellBookUnmemorizeSpell ():
	if SpellBookSpellUnmemorizeWindow:
		OpenSpellBookSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = SelectedBook
	Window = SpellBookWindow

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdateSpellBookWindow ()
		GemRB.PlaySound ("GAM_44")
		Button = Window.GetControl(index + 6)
		Button.SetAnimation ("FLASH")
	return


def OnSpellBookRemoveSpell ():
	if SpellBookSpellUnmemorizeWindow:
		OpenSpellBookSpellRemoveWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = SelectedBook

	index = GemRB.GetVar ("SpellButton")

	#remove spell from memory
	#GemRB.UnmemorizeSpell (pc, type, level, index)
	#remove spell from book
	#GemRB.RemoveSpell (pc, type, level, index)
	UpdateSpellBookWindow ()
	return

###################################################
# End of file GUISPL.py
