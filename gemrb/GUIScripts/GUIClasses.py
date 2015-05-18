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
import CreateControlDecorators

from GUIDefines import *
from MetaClasses import metaIDWrapper, metaControl

def CreateControlDecorator(func):
	wrapper = getattr(CreateControlDecorators, func.__name__, None)
	if wrapper:
		return wrapper(func)
	return func # unchanged, no wrapper exists

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
  def __del__(self):
    # don't unload tables if the _GemRB module is already unloaded at exit
    if self.ID != -1 and _GemRB:
      pass #_GemRB.Table_Unload(self.ID)
  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Symbol_GetValue,
    'Unload': _GemRB.Symbol_Unload
  }
  
class GView:
	__metaclass__ = metaControl
	methods = {
    'CreateControl': _GemRB.View_CreateControl,
	'GetFrame': _GemRB.View_GetFrame,
    'SetFrame': _GemRB.View_SetFrame,
    'SetBackground': _GemRB.View_SetBackground,
    'SetFlags': _GemRB.View_SetFlags,
	}
	
	def SetSize(self, w, h):
		r = self.GetFrame()
		self.SetFrame(r['x'], r['y'], w, h);

	def SetPos(self, x, y):
		r = self.GetFrame()
		self.SetFrame(x, y, r['w'], r['h']);

class GWindow(GView):
  __metaclass__ = metaIDWrapper
  methods = {
    'DeleteControl': _GemRB.Window_DeleteControl,
    'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
    'SetupControls': _GemRB.Window_SetupControls,
    'SetVisible': _GemRB.Window_SetVisible,
    'ShowModal': _GemRB.Window_ShowModal,
    'GetControl': _GemRB.Window_GetControl,
  }

  def __nonzero__(self):
    return self.ID != -1
  def Unload(self):
    if self.ID != -1:
      _GemRB.Window_Unload(self)
      self.ID = -1

  @CreateControlDecorator
  def CreateWorldMapControl(self, control, *args):
  	return self.CreateControl(control, IE_GUI_WORLDMAP, args[0], args[1], args[2], args[3], args[4:])

  @CreateControlDecorator
  def CreateMapControl(self, control, *args):
    return self.CreateControl(control, IE_GUI_MAP, args[0], args[1], args[2], args[3], args[4:])

  @CreateControlDecorator
  def CreateLabel(self, control, *args):
  	return self.CreateControl(control, IE_GUI_LABEL, args[0], args[1], args[2], args[3], args[4:])

  @CreateControlDecorator
  def CreateButton(self, control, IE_GUI_BUTTON, *args):
    return self.CreateControl(control, IE_GUI_BUTTON, args[0], args[1], args[2], args[3], args[4:])

  @CreateControlDecorator
  def CreateScrollBar(self, control, *args):
    return self.CreateControl(control, IE_GUI_SCROLLBAR, args[0], args[1], args[2], args[3], args[4:])

  @CreateControlDecorator
  def CreateTextArea(self, control, *args):
    return self.CreateControl(control, IE_GUI_TEXTAREA, args[0], args[1], args[2], args[3], args[4:])
  
  @CreateControlDecorator
  def CreateTextEdit(self, control, *args):
    return self.CreateControl(control, IE_GUI_EDIT, args[0], args[1], args[2], args[3], args[4:]) 

class GControl(GView):
  __metaclass__ = metaControl
  methods = {
	'AttachScrollBar': _GemRB.Control_AttachScrollBar,
    'HasAnimation': _GemRB.Control_HasAnimation,
    'SetVarAssoc': _GemRB.Control_SetVarAssoc,
    'SetAnimationPalette': _GemRB.Control_SetAnimationPalette,
    'SetAnimation': _GemRB.Control_SetAnimation,
    'QueryText': _GemRB.Control_QueryText,
    'SetText': _GemRB.Control_SetText,
    'SetTooltip': _GemRB.Control_SetTooltip,
    'SetEvent': _GemRB.Control_SetEvent,
    'SetStatus': _GemRB.Control_SetStatus,
    'SubstituteForControl': _GemRB.Control_SubstituteForControl
  }

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
    'Clear': _GemRB.TextArea_Clear,
    'SetOptions': _GemRB.TextArea_SetOptions,
    'ListResources': _GemRB.TextArea_ListResources
  }
  __slots__ = ['DefaultText']

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
    'SetAnchor': _GemRB.Button_SetAnchor,
    'SetPushOffset': _GemRB.Button_SetPushOffset,
    'SetTextColor': _GemRB.Button_SetTextColor,
    'SetState': _GemRB.Button_SetState,
    'SetPictureClipping': _GemRB.Button_SetPictureClipping,
    'SetPicture': _GemRB.Button_SetPicture,
    'SetSprite2D': _GemRB.Button_SetSprite2D,
    'SetMOS': _GemRB.Button_SetMOS,
    'SetPLT': _GemRB.Button_SetPLT,
    'SetBAM': _GemRB.Button_SetBAM,
    'SetSpellIcon': _GemRB.Button_SetSpellIcon,
    'SetItemIcon': _GemRB.Button_SetItemIcon,
    'SetActionIcon': _GemRB.Button_SetActionIcon
  }

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
