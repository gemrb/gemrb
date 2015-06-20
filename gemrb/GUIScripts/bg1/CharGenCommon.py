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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
# common character generation display code
import GemRB
import GUICommon
#import GUIClasses
from ie_stats import *
from GUIDefines import *

class CharGen:
	def __init__(self,stages,startText,importFn,resetButton=False):
		"""Sets up a character generation system.
		slot: 	index of slot for which to generate 
		stages: 	List stages
		startText:		text to set info field in stage 0
		importFn: 	function ran when import is pressed
		resetButton:	whether or not to show a reset button
		
		where each stage is defined by either an intermediate screen: 
			name: 		name
			control: 		id of the controle
			text: 		id of button text or string to use
		or a request stage
			name:			name
			script | setFn:		script name to proceed or function to request and set data for this stage
			commentFn(area):	function to append information to the text area
			unsetFn:			function to remove data for this stage
			guard			return wether or not to activate this stage 
		"""
		self.stages = stages
		self.imp = importFn
		self.showReset = resetButton
		self.step = 0
		self.window = None
		self.startText=startText
	
	def displayOverview(self):
		"""
		Sets up the primary character generation window.
		show all comments of previous stages		
		"""
		if(self.window): 
			CharGenWindow = self.window
		else:
			CharGenWindow = GemRB.LoadWindow (0, "GUICG")
	
		step = self.step
		
		#set portrait	
		PortraitButton = CharGenWindow.GetControl (12)
		PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
		PortraitName = GemRB.GetToken ("LargePortrait")
		PortraitButton.SetPicture (PortraitName, "NOPORTMD")
		PortraitButton.SetState(IE_GUI_BUTTON_LOCKED)
		
		#set stage buttons
		i = 0
		for stage in self.stages:
			if(len(stage) == 3): #short stage
				(name,control,text) = stage
				button = CharGenWindow.GetControl(control)
				button.SetText(text);
				if i == step:
					button.SetState(IE_GUI_BUTTON_ENABLED)
					button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
					button.SetEvent (IE_GUI_BUTTON_ON_PRESS, self.next)
				else:
					button.SetState(IE_GUI_BUTTON_DISABLED)
			i = i + 1
		
		#set back button	
		BackButton = CharGenWindow.GetControl (11)
		#BackButton.SetText (15416)
		if(self.step != 0):
			BackButton.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			BackButton.SetState(IE_GUI_BUTTON_DISABLED)
		BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, self.back)
		BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

		AcceptButton = CharGenWindow.GetControl (8)
		playmode = GemRB.GetVar ("PlayMode")
		if playmode>=0:
			AcceptButton.SetText (11962)
		else:
			AcceptButton.SetText (13956)

		#set scrollbar
		ScrollBar = CharGenWindow.GetControl (10)
		ScrollBar.SetDefaultScrollBar()	
		
		#set import
		ImportButton = CharGenWindow.GetControl (13)
		ImportButton.SetText (13955)
		ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
		ImportButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, self.imprt)

		#set cancel and start over
		CancelButton = CharGenWindow.GetControl (15)
		if step == 0 or not self.showReset:
			CancelButton.SetText (13727) # Cancel
		else:
			CancelButton.SetText (8159) # Start over
		CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
		CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, self.cancel)

		#set and fill overview
		TextAreaControl = CharGenWindow.GetControl (9)
		if(self.step == 0):
			TextAreaControl.SetText(self.startText)
		else:
			TextAreaControl.SetText("")
		for part in range(step):
			if(len(self.stages[part]) == 5):
				(name,setFn,commentFn,unsetFn,guard) = self.stages[part]
				if(commentFn != None):
					commentFn(TextAreaControl)
		
		#show
		CharGenWindow.SetVisible(WINDOW_VISIBLE)
		self.window = CharGenWindow
	
	def unset(self,stage):
		#print "unset" , stage
		if(len(self.stages[stage]) == 5):
			(name,setFn,commentFn,unsetFn,guard) = self.stages[stage]
			if(unsetFn != None):
				unsetFn()
	#set next script to for step, return false if the current step should be skipped
	def setScript(self):
		#print 'set',self.step
		if(len(self.stages[self.step]) == 5): #long record: script of function
			(name,setFn,commentFn,unsetFn,guardFn) = self.stages[self.step]
			if(guardFn and not guardFn()):
				return False
			if(hasattr(setFn, "__call__")):
				setFn()
			else:
				GemRB.SetNextScript(setFn)
		else: #short record: overview
			GemRB.SetNextScript ("CharGen")
		return True
		
	def cancel(self):
		"""Revert back to the first step; unset all actions."""
		#if self.window:
		#	self.window.Unload()
		#reset
		for i in range(self.step,-1,-1):
			self.unset(i)

		# if required back to main screen, otherwise reloop
		if self.step == 0 or not self.showReset:	
			self.window.Unload()
			self.window = None
			GemRB.SetNextScript ("Start")
		else:
			GemRB.SetNextScript ("CharGen")

		self.step = 0


	def imprt(self):
		"""Opens the character import window."""
		self.imp()

	def back(self):
		"""Moves to the previous step. Unsets last"""
		GUICommon.CloseOtherWindow(None)
		self.unset(self.step)
		if len(self.stages[self.step]) == 3:
			#short, return to other short
			self.step = self.step - 1
			self.unset(self.step)
			while len(self.stages[self.step]) != 3:
				self.step = self.step - 1
				self.unset(self.step)
		else:
			self.step = self.step - 1
			self.unset(self.step)

		while(not self.setScript()):
			self.step = self.step - 1
			self.unset(self.step)

	
	def next(self):
		"""Calls the next setter."""
		GUICommon.CloseOtherWindow(None)
		self.step = self.step + 1
		while(not self.setScript()):
			self.step = self.step + 1

	def close(self):
		if(self.window):
			self.window.Unload()
	
	def jumpTo(self,to):
		if type(to) == str:
			done = False
			for i in range(len(self.stages)):
				if(to == self.stages[i][0]):
					self.step = i
					done = True
			if(not done):
				raise ValueError("stage name not found: "+str(to))
		elif type(to) == int:
			if(to<0):
				to = len(self.stages)+to
			if(to<0 or to>=len(self.stages)):
				raise ValueError("stage index not found: "+str(to))
			self.step = to
			
			return
		else:
			raise ValueError("bad arg type: "+str(type(to)) + " " + str(to))
		GUICommon.CloseOtherWindow(None)
		while(not self.setScript()):
			self.step = self.step + 1
					 	
CharGenMaster = None

def back():
	"""Moves to the previous step."""
	global CharGenMaster
	CharGenMaster.back()

def next():
	"""Moves to the next step."""
	global CharGenMaster
	CharGenMaster.next()

def close():
	"""Terminates CharGen."""
	global CharGenMaster
	CharGenMaster.close()
	CharGenMaster = None

def jumpTo(stage):
	global CharGenMaster
	CharGenMaster.jumpTo(stage)

