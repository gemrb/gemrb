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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/GUIDefines.py,v 1.22 2004/09/04 12:10:26 avenger_teambg Exp $


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
IE_GUI_BUTTON_DRAGGABLE  = 0x00000080

#these bits are hardcoded in the .chu structure, don't move them
IE_GUI_BUTTON_ALIGN_LEFT = 0x00000100
IE_GUI_BUTTON_ALIGN_RIGHT= 0x00000200
IE_GUI_BUTTON_ALIGN_TOP  = 0x00000400
#end of hardcoded section

IE_GUI_BUTTON_ANIMATED   = 0x00010000
IE_GUI_BUTTON_NO_TEXT    = 0x00020000   # don't draw button label

#events
IE_GUI_BUTTON_ON_PRESS    = 0x00000000
IE_GUI_MOUSE_OVER_BUTTON  = 0x00000001
IE_GUI_MOUSE_ENTER_BUTTON = 0x00000002
IE_GUI_MOUSE_LEAVE_BUTTON = 0x00000003
IE_GUI_BUTTON_ON_SHIFT_PRESS = 0x00000004
IE_GUI_BUTTON_ON_RIGHT_PRESS = 0x00000005
IE_GUI_PROGRESS_END_REACHED = 0x01000000
IE_GUI_SLIDER_ON_CHANGE   = 0x02000000
IE_GUI_EDIT_ON_CHANGE     = 0x03000000
IE_GUI_TEXTAREA_ON_CHANGE = 0x05000000
IE_GUI_LABEL_ON_PRESS     = 0x06000000
IE_GUI_SCROLLBAR_ON_CHANGE= 0x07000000

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

global OP_SET, OP_OR, OP_NAND
OP_SET = 0
OP_OR = 1
OP_NAND = 2

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

#game strings
STR_LOADMOS  = 0

global GEMRB_VERSION
GEMRB_VERSION = -1
