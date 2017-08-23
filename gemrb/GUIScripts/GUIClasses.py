#-*-python-*-
#GemRB - Infinity Engine Emulator
#Copyright (C) 2009 The GemRB Project
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


import _GemRB
import GameCheck

from GUIDefines import *
from MetaClasses import metaIDWrapper
from GemRB import GetView, CreateView, RemoveView

def CreateScrollbarARGs(bam = None):
	bamframes = list(range(6))
	
	if GameCheck.IsBG2():
		bam = bam if bam else 'GUISCRCW'
		bamframes = [0,1,2,3,5,4]
	elif GameCheck.IsPST():
		bam = bam if bam else 'CGSCRL1'
	else:
		bam = bam if bam else 'GUIWSBR'

	return (bam, bamframes)

class GTable:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Table_GetValue,
    'FindValue': _GemRB.Table_FindValue,
    'GetRowIndex': _GemRB.Table_GetRowIndex,
    'GetRowName': _GemRB.Table_GetRowName,
    'GetColumnIndex': _GemRB.Table_GetColumnIndex,
    'GetColumnName': _GemRB.Table_GetColumnName,
    'GetRowCount': _GemRB.Table_GetRowCount,
    'GetColumnCount': _GemRB.Table_GetColumnCount
  }

  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Symbol_GetValue,
    'Unload': _GemRB.Symbol_Unload
  }

class GView:
	__metaclass__ = metaIDWrapper
	methods = {
	'AddAlias': _GemRB.View_AddAlias,
	'AddSubview': _GemRB.View_AddSubview,
	'GetFrame': _GemRB.View_GetFrame,
    'SetFrame': _GemRB.View_SetFrame,
    'SetBackground': _GemRB.View_SetBackground,
    'SetFlags': _GemRB.View_SetFlags,
	}
	__slots__ = ['SCRIPT_GROUP']

	def SetSize(self, w, h):
		r = self.GetFrame()
		r['w'] = w
		r['h'] = h
		self.SetFrame(r);

	def SetPos(self, x, y):
		r = self.GetFrame()
		r['x'] = x
		r['y'] = y
		self.SetFrame(r);

	def GetInsetFrame(self, l = 0, r = None, t = None, b = None):
		r = r if r is not None else l
		t = t if t is not None else l
		b = b if b is not None else t
        
		frame = self.GetFrame()
		frame['x'] = l
		frame['y'] = b
		frame['w'] -= (l + r)
		frame['h'] -= (b + t)
		return frame

	def SetVisible(self, visible):
		self.SetFlags(IE_GUI_VIEW_INVISIBLE, OP_NAND if visible else OP_OR)

	def SetDisabled(self, disable):
		self.SetFlags(IE_GUI_VIEW_DISABLED, OP_OR if disable else OP_NAND)

	def CreateControl(self, id, type, x, y, w, h, *args): # backwards compatibility
		frame = {"x" : x, "y" : y, "w" : w, "h" : h}
		return self.CreateSubview(id, type, frame, *args)

	def CreateSubview(self, id, type, frame, *args):
		view = CreateView(id, type, frame, *args)
		return self.AddSubview(view)

	def RemoveSubview(self, view):
		RemoveView(view, False)

	def CreateWorldMapControl(self, control, *args):
		return self.CreateControl(control, IE_GUI_WORLDMAP, args[0], args[1], args[2], args[3], args[4:])
	
	def CreateMapControl(self, control, *args):
		return self.CreateControl(control, IE_GUI_MAP, args[0], args[1], args[2], args[3], args[4:])
  
	def CreateLabel(self, control, *args):
		return self.CreateControl(control, IE_GUI_LABEL, args[0], args[1], args[2], args[3], args[4:])
  
	def CreateButton(self, control, *args):
		return self.CreateControl(control, IE_GUI_BUTTON, args[0], args[1], args[2], args[3], args[4:])
  
	def CreateScrollBar(self, control, frame, bam=None):
		view = CreateView (control, IE_GUI_SCROLLBAR, frame, CreateScrollbarARGs(bam))
		return self.AddSubview (view)
  
	def CreateTextArea(self, control, *args):
		return self.CreateControl(control, IE_GUI_TEXTAREA, args[0], args[1], args[2], args[3], args[4:])
  
	def CreateTextEdit(self, control, *args):
		return self.CreateControl(control, IE_GUI_EDIT, args[0], args[1], args[2], args[3], args[4:])

class GWindow(GView):
  methods = {
    'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
    'SetupControls': _GemRB.Window_SetupControls,
    'Focus': _GemRB.Window_Focus,
    'ShowModal': _GemRB.Window_ShowModal,
  }

  def __nonzero__(self):
    return self.ID != -1

  def DeleteControl(self, view): # backwards compatibility
	if type(view) == int:
		view = self.GetControl (view)
	RemoveView (view, True)

  def GetControl(self, id): # backwards compatibility
	return GetView(self, id)

  def Unload(self): # backwards compatibility
	self.Close()

  def Close(self):
	if self.ID != -1:
		RemoveView(self, False)
		self.ID = -1

class GControl(GView):
  methods = {
    'HasAnimation': _GemRB.Control_HasAnimation,
    'SetVarAssoc': _GemRB.Control_SetVarAssoc,
    'SetAnimationPalette': _GemRB.Control_SetAnimationPalette,
    'SetAnimation': _GemRB.Control_SetAnimation,
    'QueryText': _GemRB.Control_QueryText,
    'SetText': _GemRB.Control_SetText,
    'SetTooltip': _GemRB.Control_SetTooltip,
    'SetAction': _GemRB.Control_SetAction,
    'SetActionInterval': _GemRB.Control_SetActionInterval,
    'SetStatus': _GemRB.Control_SetStatus,
    'SubstituteForControl': _GemRB.Control_SubstituteForControl
  }

  # backwards compatibility
  # map old event identifiers to new action system
  EventMap = { IE_GUI_BUTTON_ON_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 1),
  			   IE_GUI_MOUSE_ENTER_BUTTON: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_ENTER),
  			   IE_GUI_MOUSE_LEAVE_BUTTON: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_LEAVE),
			   IE_GUI_BUTTON_ON_SHIFT_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 1, 1),
			   IE_GUI_BUTTON_ON_RIGHT_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_MENU, 0, 1),
  			   IE_GUI_BUTTON_ON_DRAG_DROP: lambda control, handler: control.SetAction(handler, IE_GUI_BUTTON_ON_DRAG_DROP),
			   IE_GUI_BUTTON_ON_DOUBLE_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2),
  			   IE_GUI_PROGRESS_END_REACHED: lambda control, handler: control.SetAction(handler, IE_GUI_PROGRESS_END_REACHED),
  			   IE_GUI_SLIDER_ON_CHANGE: lambda control, handler: control.SetAction(handler, IE_ACT_VALUE_CHANGE),
  			   IE_GUI_EDIT_ON_CHANGE: lambda control, handler: control.SetAction(handler, IE_ACT_VALUE_CHANGE),
  			   IE_GUI_EDIT_ON_DONE: lambda control, handler: control.SetAction(handler, IE_GUI_EDIT_ON_DONE),
  			   IE_GUI_EDIT_ON_CANCEL: lambda control, handler: control.SetAction(handler, IE_GUI_EDIT_ON_CANCEL),
  			   IE_GUI_TEXTAREA_ON_CHANGE: lambda control, handler: control.SetAction(handler, IE_ACT_VALUE_CHANGE),
  			   IE_GUI_TEXTAREA_ON_SELECT: lambda control, handler: control.SetAction(handler, IE_GUI_TEXTAREA_ON_SELECT),
  			   IE_GUI_LABEL_ON_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, 1, 0, 1),
  			   IE_GUI_SCROLLBAR_ON_CHANGE: lambda control, handler: control.SetAction(handler, IE_ACT_VALUE_CHANGE),
  			   IE_GUI_WORLDMAP_ON_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, 1, 0, 1),
  			   IE_GUI_MAP_ON_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, 1, 0, 1),
  			   IE_GUI_MAP_ON_RIGHT_PRESS: lambda control, handler: control.SetAction(handler, IE_ACT_MOUSE_PRESS, 2, 0, 1)
  			 }

  def SetEvent(self, event, handler):
    GControl.EventMap[event](self, handler)

class GLabel(GControl):
  methods = {
    'SetFont': _GemRB.Label_SetFont,
    'SetTextColor': _GemRB.Label_SetTextColor,
    'SetUseRGB': _GemRB.Label_SetUseRGB
  }

class GTextArea(GControl):
  methods = {
    'ChapterText': _GemRB.TextArea_SetChapterText,
    'Append': _GemRB.TextArea_Append,
    'ListResources': _GemRB.TextArea_ListResources
  }
  __slots__ = ['DefaultText']

  def Clear(self):
    self.SetText("")

  def SetOptions(self, optList, varname=None, val=0):
    _GemRB.TextArea_SetOptions(self, optList)
    if varname:
    	self.SetVarAssoc(varname, val)

class GTextEdit(GControl):
  methods = {
    'SetBufferLength': _GemRB.TextEdit_SetBufferLength,
  }

class GScrollBar(GControl):
  methods = {
    'SetDefaultScrollBar': _GemRB.ScrollBar_SetDefaultScrollBar
  }

class GButton(GControl):
  methods = {
    'SetSprites': _GemRB.Button_SetSprites,
    'SetOverlay': _GemRB.Button_SetOverlay,
    'SetBorder': _GemRB.Button_SetBorder,
    'EnableBorder': _GemRB.Button_EnableBorder,
    'SetFont': _GemRB.Button_SetFont,
    'SetHotKey': _GemRB.Button_SetHotKey,
    'SetAnchor': _GemRB.Button_SetAnchor,
    'SetPushOffset': _GemRB.Button_SetPushOffset,
    'SetTextColor': _GemRB.Button_SetTextColor,
    'SetState': _GemRB.Button_SetState,
    'SetPictureClipping': _GemRB.Button_SetPictureClipping,
    'SetPicture': _GemRB.Button_SetPicture,
    'SetPLT': _GemRB.Button_SetPLT,
    'SetBAM': _GemRB.Button_SetBAM,
    'SetSpellIcon': _GemRB.Button_SetSpellIcon,
    'SetItemIcon': _GemRB.Button_SetItemIcon,
    'SetActionIcon': _GemRB.Button_SetActionIcon
  }

  def MakeDefault(self):
	  # return key
	  return self.SetHotKey(chr(0x86))

  def MakeEscape(self):
	  # escape key
	  return self.SetHotKey(chr(0x8c))

  def SetMOS(self, mos):
	  self.SetPicture(mos) # backwards compatibility

  def SetSprite2D(self, spr):
	  self.SetPicture(spr) # backwards compatibility

  def CreateLabel(self, labelid, *args):
	  frame = self.GetFrame()
	  return self.CreateControl(labelid, IE_GUI_LABEL, 0, 0, frame['w'], frame['h'], args)

class GWorldMap(GControl):
  methods = {
    'AdjustScrolling': _GemRB.WorldMap_AdjustScrolling,
    'GetDestinationArea': _GemRB.WorldMap_GetDestinationArea,
    'SetTextColor': _GemRB.WorldMap_SetTextColor
  }

class GSaveGame:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetDate': _GemRB.SaveGame_GetDate,
    'GetGameDate': _GemRB.SaveGame_GetGameDate,
    'GetName': _GemRB.SaveGame_GetName,
    'GetPortrait': _GemRB.SaveGame_GetPortrait,
    'GetPreview': _GemRB.SaveGame_GetPreview,
    'GetSaveID': _GemRB.SaveGame_GetSaveID,
  }

class GSprite2D:
  __metaclass__ = metaIDWrapper
  methods = {}
