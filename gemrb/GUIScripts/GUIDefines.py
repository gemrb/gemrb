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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/GUIDefines.py,v 1.33 2005/03/28 08:43:37 avenger_teambg Exp $


# GUIDefines.py - common definitions of GUI-related constants for GUIScripts

#button flags
IE_GUI_BUTTON_NORMAL     = 0x00000004   #default button, doesn't stick
IE_GUI_BUTTON_NO_IMAGE   = 0x00000001
IE_GUI_BUTTON_PICTURE    = 0x00000002
IE_GUI_BUTTON_SOUND      = 0x00000004
IE_GUI_BUTTON_ALT_SOUND  = 0x00000008
IE_GUI_BUTTON_CHECKBOX   = 0x00000010   #or radio button
IE_GUI_BUTTON_RADIOBUTTON= 0x00000020   #sticks in a state
IE_GUI_BUTTON_DEFAULT    = 0x00000040   #enter button triggers it
IE_GUI_BUTTON_ANIMATED   = 0x00000080   # the button is animated

#these bits are hardcoded in the .chu structure, don't move them
IE_GUI_BUTTON_ALIGN_LEFT = 0x00000100
IE_GUI_BUTTON_ALIGN_RIGHT= 0x00000200
IE_GUI_BUTTON_ALIGN_TOP  = 0x00000400
#end of hardcoded section
IE_GUI_BUTTON_ALIGN_BOTTOM = 0x00000800

IE_GUI_BUTTON_NO_TEXT    = 0x00010000   # don't draw button label
IE_GUI_BUTTON_PLAYRANDOM = 0x00040000   # the button animation is random
IE_GUI_BUTTON_PLAYONCE   = 0x00080000   # the button animation won't restart

#textarea flags
IE_GUI_TEXTAREA_SELECTABLE = 0x05000001
IE_GUI_TEXTAREA_AUTOSCROLL = 0x05000002
IE_GUI_TEXTAREA_SMOOTHSCROLL = 0x05000004

#events
IE_GUI_BUTTON_ON_PRESS    = 0x00000000
IE_GUI_MOUSE_OVER_BUTTON  = 0x00000001
IE_GUI_MOUSE_ENTER_BUTTON = 0x00000002
IE_GUI_MOUSE_LEAVE_BUTTON = 0x00000003
IE_GUI_BUTTON_ON_SHIFT_PRESS = 0x00000004
IE_GUI_BUTTON_ON_RIGHT_PRESS = 0x00000005
IE_GUI_BUTTON_ON_DRAG_DROP   = 0x00000006
IE_GUI_PROGRESS_END_REACHED = 0x01000000
IE_GUI_SLIDER_ON_CHANGE   = 0x02000000
IE_GUI_EDIT_ON_CHANGE     = 0x03000000
IE_GUI_TEXTAREA_ON_CHANGE = 0x05000000
IE_GUI_LABEL_ON_PRESS     = 0x06000000
IE_GUI_SCROLLBAR_ON_CHANGE= 0x07000000
IE_GUI_MAP_ON_PRESS       = 0x09000000

#common states
IE_GUI_CONTROL_FOCUSED    = 0xff000080

#button states
IE_GUI_BUTTON_ENABLED    = 0x00000000
IE_GUI_BUTTON_UNPRESSED  = 0x00000000
IE_GUI_BUTTON_PRESSED    = 0x00000001
IE_GUI_BUTTON_SELECTED   = 0x00000002
IE_GUI_BUTTON_DISABLED   = 0x00000003
# Like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap
IE_GUI_BUTTON_LOCKED   = 0x00000004

#edit field states
IE_GUI_EDIT_NUMBER    =  0x030000001

#mapcontrol states (add 0x090000000 if used with SetControlStatus)
IE_GUI_MAP_NO_NOTES   =  0
IE_GUI_MAP_VIEW_NOTES =  1
IE_GUI_MAP_SET_NOTE   =  2

# !!! Keep these synchronized with Font.h !!!
IE_FONT_ALIGN_LEFT       = 0x00
IE_FONT_ALIGN_CENTER     = 0x01
IE_FONT_ALIGN_RIGHT      = 0x02
IE_FONT_ALIGN_BOTTOM     = 0x04
IE_FONT_ALIGN_TOP        = 0x10   # Single-Line and Multi-Line Text
IE_FONT_ALIGN_MIDDLE     = 0x20   #Only for single line Text
IE_FONT_SINGLE_LINE      = 0x40

global OP_SET, OP_OR, OP_NAND
OP_SET = 0
OP_OR = 1
OP_NAND = 2

# Window position anchors/alignments
# !!! Keep these synchronized with Window.h !!!
WINDOW_TOPLEFT       = 0x00
WINDOW_CENTER        = 0x01
WINDOW_ABSCENTER     = 0x02
WINDOW_RELATIVE      = 0x04
WINDOW_SCALE         = 0x08
WINDOW_BOUNDED       = 0x10

# Shadow color for ShowModal()
# !!! Keep these synchronized with Interface.h !!!
MODAL_SHADOW_NONE = 0
MODAL_SHADOW_GRAY = 1
MODAL_SHADOW_BLACK = 2

# Flags for GameSelectPC()
# !!! Keep these synchronized with Game.h !!!
SELECT_NORMAL  = 0x00
SELECT_REPLACE = 0x01
SELECT_QUIET   = 0x02

# Spell types
# !!! Keep these synchronized with Spellbook.h !!!
IE_SPELL_TYPE_PRIEST = 0
IE_SPELL_TYPE_WIZARD = 1
IE_SPELL_TYPE_INNATE = 2

# Item Flags bits
# !!! Keep these synchronized with Item.h !!!
IE_ITEM_CRITICAL     = 0x00000001
IE_ITEM_TWO_HANDED   = 0x00000002
IE_ITEM_MOVABLE      = 0x00000004
IE_ITEM_DISPLAYABLE  = 0x00000008
IE_ITEM_CURSED       = 0x00000010
IE_ITEM_NOT_COPYABLE = 0x00000020
IE_ITEM_MAGICAL      = 0x00000040
IE_ITEM_BOW          = 0x00000080
IE_ITEM_SILVER       = 0x00000100
IE_ITEM_COLD_IRON    = 0x00000200
IE_ITEM_STOLEN       = 0x00000400
IE_ITEM_CONVERSABLE  = 0x00000800
IE_ITEM_PULSATING    = 0x00001000
IE_ITEM_UNSELLABLE   = (IE_ITEM_CRITICAL | IE_ITEM_STOLEN)

# CREItem (SlotItem) Flags bits
# !!! Keep these synchronized with Inventory.h !!!
IE_INV_ITEM_IDENTIFIED    = 0x01
IE_INV_ITEM_UNSTEALABLE   = 0x02
IE_INV_ITEM_STOLEN        = 0x04
IE_INV_ITEM_UNDROPPABLE   = 0x08
# GemRB extensions
IE_INV_ITEM_ACQUIRED      = 0x10
IE_INV_ITEM_DESTRUCTIBLE  = 0x20
IE_INV_ITEM_EQUIPPED      = 0x40
IE_INV_ITEM_STACKED       = 0x80


#game constants
PARTY_SIZE = 6

#game strings
STR_LOADMOS  = 0

#game integers
SV_BPP = 0
SV_WIDTH = 1
SV_HEIGHT = 2
global GEMRB_VERSION
GEMRB_VERSION = -1
