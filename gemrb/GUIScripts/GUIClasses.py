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

class GWindow:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetRect': _GemRB.Window_GetRect,
    'SetSize': _GemRB.Window_SetSize,
    'SetFrame': _GemRB.Window_SetFrame,
    'SetPicture': _GemRB.Window_SetPicture,
    'SetPos': _GemRB.Window_SetPos,
    'HasControl': _GemRB.Window_HasControl,
    'DeleteControl': _GemRB.Window_DeleteControl,
    'ReassignControls': _GemRB.Window_ReassignControls,
    'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
    'SetupControls': _GemRB.Window_SetupControls,
    'SetKeyPressEvent': _GemRB.Window_SetKeyPressEvent,
    'SetVisible': _GemRB.Window_SetVisible,
    'ShowModal': _GemRB.Window_ShowModal,
    'Invalidate': _GemRB.Window_Invalidate,
  }
  def __nonzero__(self):
    return self.ID != -1
  def Unload(self):
    if self.ID != -1:
      _GemRB.Window_Unload(self.ID)
      self.ID = -1
  def GetControl(self, control):
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateWorldMapControl(self, control, *args):
    _GemRB.Window_CreateWorldMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateMapControl(self, control, *args):
    _GemRB.Window_CreateMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateLabel(self, control, *args):
    _GemRB.Window_CreateLabel(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateButton(self, control, *args):
    _GemRB.Window_CreateButton(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateScrollBar(self, control, *args):
    _GemRB.Window_CreateScrollBar(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateTextArea(self, control, *args):
    _GemRB.Window_CreateTextArea(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)    
  @CreateControlDecorator
  def CreateTextEdit(self, control, *args):
    _GemRB.Window_CreateTextEdit(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
 

class GControl:
  __metaclass__ = metaControl
  methods = {
    'GetRect': _GemRB.Control_GetRect,
    'HasAnimation': _GemRB.Control_HasAnimation,
    'SetVarAssoc': _GemRB.Control_SetVarAssoc,
    'SetPos': _GemRB.Control_SetPos,
    'SetSize': _GemRB.Control_SetSize,
    'SetAnimationPalette': _GemRB.Control_SetAnimationPalette,
    'SetAnimation': _GemRB.Control_SetAnimation,
    'QueryText': _GemRB.Control_QueryText,
    'SetText': _GemRB.Control_SetText,
    'SetTooltip': _GemRB.Control_SetTooltip,
    'SetEvent': _GemRB.Control_SetEvent,
    'SetFocus': _GemRB.Control_SetFocus,
    'SetStatus': _GemRB.Control_SetStatus,
  }
  def AttachScrollBar(self, scrollbar):
    if self.WinID != scrollbar.WinID:
      raise RuntimeError, "Scrollbar must be in same Window as Control"
    return _GemRB.Control_AttachScrollBar(self.WinID, self.ID, scrollbar.ID)
  def SubstituteForControl(self, target):
	  return _GemRB.Control_SubstituteForControl(self.WinID, self.ID, target.WinID, target.ID)

class GLabel(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetFont': _GemRB.Label_SetFont,
    'SetTextColor': _GemRB.Label_SetTextColor,
    'SetUseRGB': _GemRB.Label_SetUseRGB
  }

class GTextArea(GControl):
  __metaclass__ = metaControl
  methods = {
    'ChapterText': _GemRB.TextArea_SetChapterText,
    'Append': _GemRB.TextArea_Append,
    'Clear': _GemRB.TextArea_Clear,
    'SetFlags': _GemRB.TextArea_SetFlags,
    'ListResources': _GemRB.TextArea_ListResources
  }
  
  def SetOptions(self, optList, varname=None, val=0):
    _GemRB.TextArea_SetOptions(self.WinID, self.ID, optList)
    if varname:
    	self.SetVarAssoc(varname, val)

class GTextEdit(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetBufferLength': _GemRB.TextEdit_SetBufferLength,
    'SetBackground': _GemRB.TextEdit_SetBackground
  }

class GScrollBar(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetDefaultScrollBar': _GemRB.ScrollBar_SetDefaultScrollBar
  }

class GButton(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetSprites': _GemRB.Button_SetSprites,
    'SetOverlay': _GemRB.Button_SetOverlay,
    'SetBorder': _GemRB.Button_SetBorder,
    'EnableBorder': _GemRB.Button_EnableBorder,
    'SetFont': _GemRB.Button_SetFont,
    'SetAnchor': _GemRB.Button_SetAnchor,
    'SetPushOffset': _GemRB.Button_SetPushOffset,
    'SetTextColor': _GemRB.Button_SetTextColor,
    'SetFlags': _GemRB.Button_SetFlags,
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
  def CreateLabelOnButton(self, control, *args):
    _GemRB.Button_CreateLabelOnButton(self.WinID, self.ID, control, *args)
    return _GemRB.Window_GetControl(self.WinID, control)

class GWorldMap(GControl):
  __metaclass__ = metaControl
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
