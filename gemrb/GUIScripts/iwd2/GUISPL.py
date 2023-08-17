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

BookNames = (1083,1079,1080,1078,1077,32,1081,39722)

def InitSpellBookWindow (Window):
	#setup level buttons
	# looping backwards so the selected button gets drawn properly
	for i in range (8, -1, -1):
		Button = Window.GetControl (55 + i)
		Button.OnPress (lambda: UpdateSpellBookWindow(Window))
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("SpellBookSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (24):
		Button = Window.GetControl (6 + i)
		color = {'r' : 0, 'g' : 0, 'b' : 0, 'a' : 160}
		Button.SetBorder (0,color,0,1)
		#Button.SetBAM ("SPELFRAM",0,0,0)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetVarAssoc("Memorized", i)

	# Setup book spells buttons
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.OnRightPress (OpenSpellBookSpellInfoWindow)

	# Setup book selection
	for i in range(3, -1, -1):
		Button = Window.GetControl (88 + i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("SelectedBook", i)
		Button.OnPress (ChangeBook)

	# book left / right
	Button = Window.GetControl (92)
	Button.OnPress (SpellBookCycle)
	Button = Window.GetControl (93)
	Button.OnPress (SpellBookCycle)

	# scrollbar for known spells
	ScrollBar = Window.GetControl (54)
	Window.SetEventProxy(ScrollBar)
	ScrollBar.OnChange (lambda: UpdateSpellBookWindow(Window))

	return

def NewSpellBookWindow(Window):
	Window.AddAlias("WIN_SPL")
	InitSpellBookWindow(Window)
	GemRB.SetVar ("SpellBookSpellLevel", 0)

def GetActiveSpellBooks(pc):
	return [i for i in range(8) if GemRB.GetMemorizableSpellsCount(pc, i, 0) > 0]
	
def ChangeBook(btn):
	UpdateSpellBookWindow (btn.Window)

def GetBookType(pc):
	BookIndex = GemRB.GetVar ("SelectedBook")
	ActiveSpellBooks = GetActiveSpellBooks(pc)
	return ActiveSpellBooks[BookIndex]

def SelectedNewPlayer (Window):
	ScrollBar = Window.GetControl (54)
	ScrollBar.ScrollTo (0, 0)
	UpdateSpellBookWindow (Window)
	return

ToggleSpellBookWindow = GUICommonWindows.CreateTopWinLoader(2, "GUISPL", GUICommonWindows.ToggleWindow, NewSpellBookWindow, SelectedNewPlayer, True)
OpenSpellBookWindow = GUICommonWindows.CreateTopWinLoader(2, "GUISPL", GUICommonWindows.OpenWindowOnce, NewSpellBookWindow, SelectedNewPlayer, True)

def SpellBookCycle (btn):
	Button = btn.Window.GetControl (88 + btn.Value)
	Button.SetState (IE_GUI_BUTTON_SELECTED)
	UpdateSpellBookWindow (btn.Window)
	return

def UpdateSpellBookWindow (Window):
	pc = GemRB.GameGetSelectedPCSingle ()
	ActiveSpellBooks = GetActiveSpellBooks(pc)
	NumBooks = len(ActiveSpellBooks)

	# update spellbook buttons
	pc = GemRB.GameGetSelectedPCSingle ()
	ActiveSpellBooks = GetActiveSpellBooks (pc)
	for i in range(3, -1, -1):
		Button = Window.GetControl (88 + i)
		if len(ActiveSpellBooks) > i:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			BookType = ActiveSpellBooks[i]
			Button.SetText (BookNames[BookType])
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")

	if len(ActiveSpellBooks) == 0:
		GemRB.SetVar ("SelectedBook", None)
	else:
		Button = Window.GetControl (88)
		Button.SetState (IE_GUI_BUTTON_SELECTED)

	BookIndex = GemRB.GetVar ("SelectedBook")
	if BookIndex is not None:
		Button = Window.GetControl (92)
		Button.SetDisabled(BookIndex <= 0)
		Button.SetValue(BookIndex - 1)

		Button = Window.GetControl (93)
		Button.SetDisabled(BookIndex >= NumBooks - 1)
		Button.SetValue(BookIndex + 1)
		BookType = ActiveSpellBooks[BookIndex]
	else:
		print ("no book selected")
		BookType = 0

	level = GemRB.GetVar("SpellBookSpellLevel")
	sorcerer_style = (BookType == IE_IWD2_SPELL_BARD) or (BookType == IE_IWD2_SPELL_SORCERER)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0xfffffff)
	Label.SetText (Name)

	Button = Window.GetControl (1)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc, 0)["Sprite"])
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	memorized = MemorizedSpellList(pc, BookType, level)
	mem_cnt = len(memorized)

	for i in range (24):
		Button = Window.GetControl (6 + i)
		if GemRB.GetVar("{}_ANIM".format(Button.ControlID)): # we have an animation in progress, bail and it will refresh us after its done
			Button.SetFlags(IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
			Button.SetVisible(True)
			continue
		else:
			Button.SetFlags(IE_GUI_VIEW_IGNORE_EVENTS, OP_NAND)

		Label = Window.GetControl (0x1000003f+i)
		# actually, it doesn't display any memorized spells for sorcerer-style spellbooks
		if i < mem_cnt and not sorcerer_style:
			ms = memorized[i]
			spell = ms['SpellResRef']
			Button.SetSpellIcon (spell)
			Button.SetTooltip (ms['SpellName'])
			# since spells are stacked, we need to check first whether to unmemorize (deplete) or remove (already depleted)
			if ms['MemoCount'] < ms['KnownCount']:
				# already depleted, just remove
				Button.OnPress (lambda btn, mc = ms['MemoCount']: OnSpellBookUnmemorizeSpell(btn, mc))
			else:
				# deplete and remove
				Button.OnPress (OpenSpellBookSpellRemoveWindow)
			Button.OnRightPress (OpenSpellBookSpellInfoWindow)
			tmp = str(ms['MemoCount'])+"/"+str(ms['KnownCount'])
			Label.SetText (tmp)
			Button.SetVisible(True)
		else:
			Button.SetVisible(False)
			Label.SetText ('')

	known = KnownSpellList(pc, BookType, level)
	known_cnt = len(known)
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex") or 0
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Label = Window.GetControl (0x10000025 + i)
		SpellIdx = i + SpellTopIndex
		if SpellIdx < known_cnt:
			ks = known[SpellIdx]
			spell = ks['SpellResRef']
			Button.SetSpellIcon (spell)
			Button.SetValue(SpellIdx)
			if not sorcerer_style:
				Button.OnPress (OnSpellBookMemorizeSpell)
			Label.SetText (ks['SpellName'])
			Button.SetVisible(True)
		else:
			Button.SetVisible(False)
			Label.SetText("")

	# number of available spell slots
	# sorcerer-style books are different, since max_mem_cnt holds the total level capacity, not the slot count
	# and they display just the current and total number of memorizations per level
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, BookType, level)
	mem_cnt = GetMemorizedSpellsCount (memorized) # count of unique memorized spells
	true_mem_cnt = GemRB.GetMemorizedSpellsCount (pc, BookType, level, True)
	Label = Window.GetControl (0x10000004)
	if sorcerer_style:
		available = "0"
		if known_cnt:
			available = str(true_mem_cnt // known_cnt)
		Label.SetText (available+"/"+str(max_mem_cnt))
	else:
		# reset mem_cnt to take into account stacks
		mem_cnt = GetMemorizedSpellsCount (memorized, True)
		Label.SetText (str(max_mem_cnt-mem_cnt))

	#if actor is uncontrollable, make this grayed
	CantCast = GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	ScrollBar = Window.GetControl (54)
	ScrollBar.SetVarAssoc ("SpellTopIndex", SpellTopIndex, 0, known_cnt - 8)

	return

def GetMemorizedSpellsCount (memorized, total = False):
	'''count the real number of spells in MemorizedSpellList'''
	# can't use len here, since there could be more than one memorization (a "stack")
	# put the counts into a list and sum it, but be careful about depleted spells
	# eg. fully depleted should still count as 1 and each partial too
	count = 'MemoCount' # non-depleted only
	if total:
		count = 'KnownCount' # all

	counts = [max(x[count], 1) for x in memorized]
	return sum(counts)

def MemorizedSpellList (pc, SelectedBook, SpellBookSpellLevel):
	return Spellbook.GetMemorizedSpells (pc, SelectedBook, SpellBookSpellLevel)

def KnownSpellList (pc, SelectedBook, SpellBookSpellLevel):
	return Spellbook.GetKnownSpellsLevel (pc, SelectedBook, SpellBookSpellLevel)

def OpenSpellBookSpellInfoWindow (btn):
	Window = GemRB.LoadWindow (3, "GUISPL")

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.MakeEscape()
	Button.OnPress (Window.Close)

	pc = GemRB.GameGetSelectedPCSingle ()
	level = GemRB.GetVar("SpellBookSpellLevel")
	BookType = GetBookType(pc)

	if btn.VarName == "Memorized":
		ms = GemRB.GetMemorizedSpell (pc, BookType, level, btn.Value)
		ResRef = ms['SpellResRef']
	else:
		known = KnownSpellList(pc, BookType, level)
		ResRef = known[btn.Value]["SpellResRef"]
	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnSpellBookMemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = GemRB.GetVar("SpellBookSpellLevel")
	BookType = GetBookType(pc)
	Window = GemRB.GetView("WIN_SPL")

	def Complete():
		mem_cnt = GemRB.GetMemorizedSpellsCount(pc, BookType, level, False)
		AnimBtn = Window.GetControl(mem_cnt + 5)
		AnimBtn.SetAnimation("FLASH", 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		AnimBtn.OnAnimEnd(lambda: UpdateSpellBookWindow (Window))

	if GemRB.MemorizeSpell (pc, BookType, level, btn.Value):
		GemRB.PlaySound ("GAM_24")
		btn.SetAnimation ("FLASH", 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		btn.OnAnimEnd(Complete)
		UpdateSpellBookWindow (Window)

	return

def OpenSpellBookSpellRemoveWindow (btn):
	Window = GemRB.LoadWindow (5, "GUISPL")

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetValue(btn.Value)
	Button.OnPress (OnSpellBookRemoveSpell)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (lambda: Window.Close())
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

# since we can have semidepleted stacks, first make sure we unmemorize the depleted ones, only then unmemorize
def OnSpellBookUnmemorizeSpell (btn, mem_cnt):
	# remove one depleted spell without touching the still memorized
	# we'd need a different spell index, but it's all handled in the core
	UnmemoSpell(btn, mem_cnt != 0)
	return

# not like removing spells in bg2, where you could delete known spells from the spellbook!
def OnSpellBookRemoveSpell (btn):
	UnmemoSpell (btn)
	btn.Window.Close()
	return

def UnmemoSpell (btn, onlydepleted = False):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = GemRB.GetVar("SpellBookSpellLevel")
	BookType = GetBookType(pc)

	Window = GemRB.GetView("WIN_SPL")
	def Complete(btn):
		# remove spell from memory
		GemRB.UnmemorizeSpell (pc, BookType, level, btn.Value, onlydepleted)
		UpdateSpellBookWindow(Window)

	GemRB.PlaySound ("GAM_44")
	AnimBtn = Window.GetControl(6 + btn.Value)
	AnimBtn.SetAnimation ("FLASH", 0, A_ANI_PLAYONCE | A_ANI_BLEND)
	AnimBtn.OnAnimEnd(Complete)

###################################################
# End of file GUISPL.py
