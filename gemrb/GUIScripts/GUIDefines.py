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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIDefines.py - common definitions of GUI-related constants for GUIScripts

#button flags
IE_GUI_BUTTON_NORMAL     = 0x00000004   #default button, doesn't stick
IE_GUI_BUTTON_NO_IMAGE   = 0x00000001
IE_GUI_BUTTON_PICTURE    = 0x00000002
IE_GUI_BUTTON_SOUND      = 0x00000004
IE_GUI_BUTTON_ALT_SOUND  = 0x00000008
IE_GUI_BUTTON_CHECKBOX   = 0x00000010   #or radio button
IE_GUI_BUTTON_RADIOBUTTON= 0x00000020   #sticks in a state
IE_GUI_BUTTON_DEFAULT    = 0x00000040   #enter key triggers it
IE_GUI_BUTTON_ANIMATED   = 0x00000080   # the button is animated

#these bits are hardcoded in the .chu structure, don't move them
IE_GUI_BUTTON_ALIGN_LEFT = 0x00000100
IE_GUI_BUTTON_ALIGN_RIGHT= 0x00000200
IE_GUI_BUTTON_ALIGN_TOP  = 0x00000400
IE_GUI_BUTTON_ALIGN_BOTTOM = 0x00000800
IE_GUI_BUTTON_ALIGN_ANCHOR = 0x00001000
IE_GUI_BUTTON_LOWERCASE    = 0x00002000
IE_GUI_BUTTON_MULTILINE    = 0x00004000 # don't set the SINGLE_LINE font rendering flag
#end of hardcoded section

IE_GUI_BUTTON_DRAGGABLE    = 0x00008000
IE_GUI_BUTTON_NO_TEXT    = 0x00010000   # don't draw button label
IE_GUI_BUTTON_PLAYRANDOM = 0x00020000   # the button animation is random
IE_GUI_BUTTON_PLAYONCE   = 0x00040000   # the button animation won't restart

IE_GUI_BUTTON_CENTER_PICTURES = 0x00080000 # center the button's PictureList
IE_GUI_BUTTON_BG1_PAPERDOLL   = 0x00100000 # BG1-style paperdoll
IE_GUI_BUTTON_HORIZONTAL      = 0x00200000 # horizontal clipping of overlay
IE_GUI_BUTTON_CANCEL          = 0x00400000 # escape key triggers it
IE_GUI_BUTTON_CAPS            = 0x00800000 # capitalize all the text (default for bg2)

#scrollbar flags
IE_GUI_SCROLLBAR_DEFAULT = 0x00000040   # mousewheel triggers it (same value as default button)

#textarea flags
IE_GUI_TEXTAREA_SELECTABLE = 0x05000001
IE_GUI_TEXTAREA_AUTOSCROLL = 0x05000002
IE_GUI_TEXTAREA_SMOOTHSCROLL = 0x05000004
IE_GUI_TEXTAREA_HISTORY = 0x05000008
IE_GUI_TEXTAREA_SPEAKER = 0x05000010
IE_GUI_TEXTAREA_EDITABLE = 0x05000040

#gui control types
IE_GUI_BUTTON = 0
IE_GUI_PROGRESS = 1
IE_GUI_SLIDER = 2
IE_GUI_EDIT = 3
IE_GUI_TEXTAREA = 5
IE_GUI_LABEL = 6
IE_GUI_SCROLLBAR = 7
IE_GUI_WORLDMAP = 8
IE_GUI_MAP = 9

#events
IE_GUI_BUTTON_ON_PRESS      = 0x00000000
IE_GUI_MOUSE_OVER_BUTTON    = 0x00000001
IE_GUI_MOUSE_ENTER_BUTTON   = 0x00000002
IE_GUI_MOUSE_LEAVE_BUTTON   = 0x00000003
IE_GUI_BUTTON_ON_SHIFT_PRESS= 0x00000004
IE_GUI_BUTTON_ON_RIGHT_PRESS= 0x00000005
IE_GUI_BUTTON_ON_DRAG_DROP  = 0x00000006
IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT = 0x00000007
IE_GUI_BUTTON_ON_DRAG       = 0x00000008
IE_GUI_BUTTON_ON_DOUBLE_PRESS = 0x00000009
IE_GUI_PROGRESS_END_REACHED = 0x01000000
IE_GUI_SLIDER_ON_CHANGE     = 0x02000000
IE_GUI_EDIT_ON_CHANGE       = 0x03000000
IE_GUI_EDIT_ON_DONE         = 0x03000001
IE_GUI_EDIT_ON_CANCEL       = 0x03000002
IE_GUI_TEXTAREA_ON_CHANGE   = 0x05000000
IE_GUI_LABEL_ON_PRESS       = 0x06000000
IE_GUI_SCROLLBAR_ON_CHANGE  = 0x07000000
IE_GUI_WORLDMAP_ON_PRESS    = 0x08000000
IE_GUI_MOUSE_ENTER_WORLDMAP = 0x08000002
IE_GUI_MAP_ON_PRESS         = 0x09000000
IE_GUI_MAP_ON_RIGHT_PRESS   = 0x09000005
IE_GUI_MAP_ON_DOUBLE_PRESS  = 0x09000008

#common states
IE_GUI_CONTROL_FOCUSED    = 0x7f000080

#button states
IE_GUI_BUTTON_ENABLED    = 0x00000000
IE_GUI_BUTTON_UNPRESSED  = 0x00000000
IE_GUI_BUTTON_PRESSED    = 0x00000001
IE_GUI_BUTTON_SELECTED   = 0x00000002
IE_GUI_BUTTON_DISABLED   = 0x00000003
# Like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap
IE_GUI_BUTTON_LOCKED   = 0x00000004
# Draws DISABLED bitmap, but it isn't disabled
IE_GUI_BUTTON_FAKEDISABLED    = 0x00000005
# Draws PRESSED bitmap, but it isn't shifted
IE_GUI_BUTTON_FAKEPRESSED   = 0x00000006

#edit field states
IE_GUI_EDIT_NUMBER    =  0x030000001

#mapcontrol states (add 0x090000000 if used with SetControlStatus)
IE_GUI_MAP_NO_NOTES   =  0
IE_GUI_MAP_VIEW_NOTES =  1
IE_GUI_MAP_SET_NOTE   =  2
IE_GUI_MAP_REVEAL_MAP =  3

# !!! Keep these synchronized with WorldMapControl.h !!!
# WorldMap label colors
IE_GUI_WMAP_COLOR_BACKGROUND = 0
IE_GUI_WMAP_COLOR_NORMAL     = 1
IE_GUI_WMAP_COLOR_SELECTED   = 2
IE_GUI_WMAP_COLOR_NOTVISITED = 3

# !!! Keep these synchronized with Font.h !!!
IE_FONT_ALIGN_LEFT       = 0x00
IE_FONT_ALIGN_CENTER     = 0x01
IE_FONT_ALIGN_RIGHT      = 0x02
IE_FONT_ALIGN_BOTTOM     = 0x04
IE_FONT_ALIGN_TOP        = 0x10   # Single-Line and Multi-Line Text
IE_FONT_ALIGN_MIDDLE     = 0x20   #Only for single line Text
IE_FONT_SINGLE_LINE      = 0x40

OP_SET = 0
OP_AND = 1
OP_OR = 2
OP_XOR = 3
OP_NAND = 4

# Window position anchors/alignments
# !!! Keep these synchronized with Window.h !!!
WINDOW_TOPLEFT       = 0x00
WINDOW_CENTER        = 0x01
WINDOW_ABSCENTER     = 0x02
WINDOW_RELATIVE      = 0x04
WINDOW_SCALE         = 0x08
WINDOW_BOUNDED       = 0x10

# GameScreen flags
GS_PARTYAI           = 1
GS_SMALLDIALOG       = 0
GS_MEDIUMDIALOG      = 2
GS_LARGEDIALOG       = 6
GS_DIALOGMASK        = 6
GS_DIALOG            = 8
GS_HIDEGUI           = 16
GS_OPTIONPANE        = 32
GS_PORTRAITPANE      = 64
GS_MAPNOTE           = 128

# GameControl screen flags
# !!! Keep these synchronized with GameControl.h !!!
SF_DISABLEMOUSE      = 1
SF_CENTERONACTOR     = 2
SF_ALWAYSCENTER      = 4
SF_GUIENABLED        = 8
SF_LOCKSCROLL        = 16

# GameControltarget modes
# !!! Keep these synchronized with GameControl.h !!!
TARGET_MODE_NONE    = 0
TARGET_MODE_TALK    = 1
TARGET_MODE_ATTACK  = 2
TARGET_MODE_CAST    = 3
TARGET_MODE_DEFEND  = 4
TARGET_MODE_PICK    = 5

GA_SELECT     = 16
GA_NO_DEAD    = 32
GA_POINT      = 64
GA_NO_HIDDEN  = 128
GA_NO_ALLY    = 256
GA_NO_ENEMY   = 512
GA_NO_NEUTRAL = 1024
GA_NO_SELF    = 2048

# Shadow color for ShowModal()
# !!! Keep these synchronized with Interface.h !!!
MODAL_SHADOW_NONE = 0
MODAL_SHADOW_GRAY = 1
MODAL_SHADOW_BLACK = 2

# Flags for SetVisible()
# !!! Keep these synchronized with Interface.h !!!
#WINDOW_INVALID = -1
WINDOW_INVISIBLE = 0
WINDOW_VISIBLE = 1
WINDOW_GRAYED = 2
WINDOW_FRONT = 3

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

# IWD2 spell types
IE_IWD2_SPELL_BARD = 0
IE_IWD2_SPELL_CLERIC = 1
IE_IWD2_SPELL_DRUID = 2
IE_IWD2_SPELL_PALADIN = 3
IE_IWD2_SPELL_RANGER = 4
IE_IWD2_SPELL_SORCEROR = 5
IE_IWD2_SPELL_WIZARD = 6
IE_IWD2_SPELL_DOMAIN = 7
IE_IWD2_SPELL_INNATE = 8
IE_IWD2_SPELL_SONG = 9
IE_IWD2_SPELL_SHAPE = 10

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
# these come from the original item bits
IE_INV_ITEM_CRITICAL      = 0x100
IE_INV_ITEM_TWOHANDED     = 0x200
IE_INV_ITEM_MOVABLE       = 0x400
IE_INV_ITEM_UNKNOWN800    = 0x800
IE_INV_ITEM_CURSED        = 0x1000
IE_INV_ITEM_UNKNOWN2000   = 0x2000
IE_INV_ITEM_MAGICAL       = 0x4000
IE_INV_ITEM_BOW           = 0x8000
IE_INV_ITEM_SILVER        = 0x10000
IE_INV_ITEM_COLDIRON      = 0x20000
IE_INV_ITEM_STOLEN2       = 0x40000
IE_INV_ITEM_CONVERSIBLE   = 0x80000
IE_INV_ITEM_PULSATING     = 0x100000

#repeat key flags
GEM_RK_DOUBLESPEED = 1
GEM_RK_DISABLE = 2
GEM_RK_QUADRUPLESPEED = 4

SHOP_BUY    = 1
SHOP_SELL   = 2
SHOP_ID     = 4
SHOP_STEAL  = 8
SHOP_SELECT = 0x40

#game constants
PARTY_SIZE = 6

# !!! Keep this synchronized with Video.h !!!
TOOLTIP_DELAY_FACTOR = 250

#game strings
STR_LOADMOS  = 0
STR_AREANAME = 1

#game integers
SV_BPP = 0
SV_WIDTH = 1
SV_HEIGHT = 2
global GEMRB_VERSION
GEMRB_VERSION = -1

# GUIEnhancements bits
GE_SCROLLBARS = 1
GE_TRY_IDENTIFY_ON_TRANSFER = 2
GE_ALWAYS_OPEN_CONTAINER_ITEMS = 4

# Log Levels
# !!! Keep this synchronized with System/Logging.h !!!
# no need for LOG_INTERNAL here since its internal to the logger class
LOG_NONE = -1 # here just for the scripts, not needed in core
LOG_FATAL = 0
LOG_ERROR = 1
LOG_WARNING = 2
LOG_MESSAGE = 3
LOG_COMBAT = 4
LOG_DEBUG = 5
