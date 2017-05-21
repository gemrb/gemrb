from GUIDefines import *
#import CommonWindow
import GemRB
GemOptionsWindow = None
import GameCheck
def TrySavingConfiguration():
	if not GemRB.SaveConfig():
		print "ARGH, could not write config to disk!!"
###################################################

# The following functions generate a simple menu for
# dealing with GemRB Specific Options

# For basic 'boolean options', just add them to an appropriate
# dictionary in the manner below:
# todo: options that are commented out have not been verified
# to work for runtime toggling
#DebugOptions = { #In game text : INI key
#	'Skip startup videos' : 'SkipIntroVideos',
#	'Show framerate monitor' : 'DrawFPS',
#	'Disable/Enable?! Fog of War': 'FogOfWar',
#	'Enable debug keys' : 'EnableCheatKeys'
#	 }
#ControlOptions = {
#	'Mouse cursor display feedback' : 'MouseFeedback', #requires special controls
#	'Touchscreen edge scroll area' : 'TouchScrollAreas' #is only toggled when game reloaded
#	}

# If an option has a range of values, enter it here. default is the
# maximum value. if the option must have a minimum value, eg 20 to 100,
# enter it as:
# 	IniKey : 100,  IniKeyMin : 20,
# if required, it could be expanded to add custom
# controls, eg:
# 	AwesomeLevel : 100, AwesomeLevelControl: Slider, AwesomeLevelMin : 99
# todo: implement actual controls using logic in PrintOptions()
#SpecialValues = {
#	"MouseFeedback" : 3,
#	"MouseFeedbackMin": 0, #duh, but anyway
#	"MouseFeedbackControl": "Slider"
#	}

# Toggle buttons will be create to change the GUIEnh flag
# for any option that has an integer instead of an ini string:
UIOptions = { #In game text : Flag Value
	'Extend ui with scrollbars when needed' : GE_SCROLLBARS
	}
LootOptions = {
	'Automatically identify loot' : GE_TRY_IDENTIFY_ON_TRANSFER,
	'Open bags directly' : GE_ALWAYS_OPEN_CONTAINER_ITEMS,
	'Automatically bag loot' : GE_TRY_TO_BAG_LOOT
	}
# Test options: uncomment should you need to test the visual layout of
# the menu:
#	'Extra action buttons': GE_EXTRA_ACTION_BUTTONS,
#	'Switch weapons from action bar': GE_SELECT_WEAPON_FROM_ACTION_BAR,
#	'Quick load on splash screen' : GE_SPLASH_LOAD_SHORTCUT,
#	'Condensed character sheet' : GE_COMPACT_RECORD_SHEET,
#	'Override UI layout' : GE_OVERRIDE_CHU_POSITIONS,
#	'Make Toast' : 256,
#	'Enable Orbital Cannons': 512,
#	'All familiars are Elder Dragons': 1024,
#	'Enable Jumping': 2048,
#	'Automatically convert rules to 3rd edition': 4096,
#	'This option has a description that is superfluous in regard to its non-existent effects' : 8192,
#	'Predict Lottery Numbers': 16384,
#	'The answer to Life, The Universe, and Everything': 32768
#	}

#The options should be able to go in arbitrary groups
#This is helpful because dictionaries are in a random order
OptionGroups = UIOptions, LootOptions#, DebugOptions, ControlOptions

#Finally, there needs to be a global to store the page number:
GemOptionsPage = 0

def ToggleGemRBFlag(): #toggles the state of a GUIEhn Flag

	guien = GemRB.GetVar("GUIEnhancements")
	flag = GemRB.GetVar("GmFlag")
	guien ^= flag
	GemRB.SetVar("GUIEnhancements", guien)
	RefreshGemRBOptionsWindow ()

#The GUI Engine cannot attach an Ini variable name to a button,
#so in order to work out which option to toggle, we must regenerate
#a 'virtual' options window and stop at the corresponding control
def ToggleIniOption():

	global GemOptionsPage

	ci = int(GemRB.GetVar("GmFlag"))
	targetop = 0
	si = GemOptionsPage*16 #skip multiples of 16

	for g in range(len(OptionGroups)):
		group = OptionGroups[g]
		for key in group:
			if si:
				si -=1
				continue

			option = str(group[key])
			if option.isdigit(): #this can only mean a guiflag instead of ini key
				targetop +=1
				continue
			elif targetop == ci:
				val = not GemRB.GetVar(option)
				GemRB.SetVar( option , int(val))
				RefreshGemRBOptionsWindow ()
				return
			targetop +=1

def RefreshGemRBOptionsWindow (): #refreshes the visual state of the buttons

	global GemOptionsWindow, GemOptionsPage

	Window = GemOptionsWindow
	ci = 0	# control index, assumes 0-15
	si = GemOptionsPage*16 	# option list start index
	guien = GemRB.GetVar("GUIEnhancements")
	for g in range(len(OptionGroups)):
		group = OptionGroups[g]
		for key in group:
			if si:
				si -=1		# skip until at the start index
				continue
			if not Window.HasControl(ci):
				return #no point trying to create empty buttons
			Button = Window.GetControl(ci)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			option = str(group[key])
			if option.isdigit(): #this can only mean a guiflag instead of ini key
				Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ToggleGemRBFlag)
				Button.SetVarAssoc("GmFlag", group[key])
				if guien&group[key]:
					Button.SetState(IE_GUI_BUTTON_SELECTED)
			else: # every other ini value should be set here
				Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ToggleIniOption)
				Button.SetVarAssoc("GmFlag", ci)
				if GemRB.GetVar(option):
					Button.SetState(IE_GUI_BUTTON_SELECTED)
			Label = Window.GetControl(100+ci)
			Label.SetText(key)

			ci +=1
			if ci == 16:
				return

	while ci < 16: #flush remaining buttons
		if not Window.HasControl(ci):
			return #no point trying to create empty buttons

		Button = Window.GetControl(ci)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		#todo: if there is a simple way to render a button invisible,
		#that would look a lot better
		Label = Window.GetControl(100+ci)
		Label.SetText("")
		ci+=1
		if ci == 16:
			return True ##if there are no more options, tell the pager

def PageGemRBOptionsWindow ():

	global GemOptionsWindow, GemOptionsPage
	GemOptionsPage = GemRB.GetVar("GmFlag")

	if GemOptionsPage > 0:
		Button = GemOptionsWindow.GetControl(94)
		Button.SetVarAssoc("GmFlag", GemOptionsPage-1)

	if not RefreshGemRBOptionsWindow():
		Button = GemOptionsWindow.GetControl(96)
		Button.SetVarAssoc("GmFlag", GemOptionsPage+1)


def ToggleGemRBOptionsWindow (): #toggle, Toggle, TOGGLE!

	global GemOptionsWindow, GameOptionsWindow, GemOptionsPage

	PrintOptions()

	if GemOptionsWindow:
		GemOptionsWindow.SetVisible(WINDOW_INVISIBLE)
		GemOptionsWindow.Unload () #does this kill the internal window like it needs to?
		GemOptionsWindow = None
		GemOptionsPage = 0
		TrySavingConfiguration ()
		return

	#some game specific visual adjustment:
	menuback = "GUIMAPAB"
	bgfx = "TOGGLE" , 0,0,1,3,2 #button bam
	fonts = "REALMS" , "NORMAL"
	tco = 30, 24, 210, 24 #title coords
	xco = 512-45, 12 #close coords
	po = 130 #panel offseet
	if GameCheck.IsIWD2():
		menuback = "GIITMH08"
		tco = 30, 43, 210, 24
		xco = 512-44, 20
		po = 118
	elif GameCheck.IsBG2():
		menuback = "GUIMGCP"
		bgfx = "ROUNDBUT", 0,3,1,2,4
		xco = 512-70, 10
		tco = 20, 10, 300, 24
		po = 105
	elif GameCheck.IsPST():
		menuback = "MAPANEL" # "WMMOSX""BPMOS1""WMMOSFM"
		bgfx = "GPERBUT4", 0,0,2,3,0
		fonts = "EXOFONT", "FONTDLG"
		tco = 10, 24, 210, 24
		xco = 526 - 48, 24
		po = 70

	GemRB.CreateWindow(19,64,0,526,480,menuback) #slightly over 512 for sake of torment
	GemOptionsWindow = GemRB.LoadWindow(19)

	Window = GemOptionsWindow

	#Title:
	Window.CreateLabel(99, tco[0],tco[1],tco[2],tco[3], fonts[0], "GEMRB OPTIONS", IE_FONT_ALIGN_CENTER)
	#Close button:
	Button = Window.CreateButton(98, xco[0], xco[1], 24, 24) #iwd2
	Button.SetSprites(bgfx[0],bgfx[1],bgfx[2],bgfx[3],bgfx[4],bgfx[5])
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ToggleGemRBOptionsWindow)
	Window.CreateLabel(97, xco[0]-38, xco[1], 40, 24, fonts[1], "Close", IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE)

	#create the toggle buttons for each flag:
	ci = 0 #control index
	si = GemOptionsPage*16
	bs = 30 #button square size
	by = 0 #vertical offset
	bx = 35 #horizontal offset

	for g in range(len(OptionGroups)):
		group = OptionGroups[g]
		for key in group:
			option = str(group[key])

			if ci == 8: #start next column
				bx = 255
				by = 0
			elif ci == 16: #if more than 16, break into paging mode
				break
			if si: # if in paging mode, skip to start index
				si -=1
				continue

			Button = Window.CreateButton(ci, bx, po+by,18,18)
			Button.SetSprites(bgfx[0],bgfx[1],bgfx[2],bgfx[3],bgfx[4],bgfx[5])
			Label = Window.CreateLabel(100+ci, bx+bs,po-5+by,180,33, fonts[1], "", IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_MIDDLE)

			ci +=1
			by += bs + 9

	if ci == 16: #todo:
		#the position of these buttons is not that nice
		#but that is academic because there aren't enough options
		#to warrant paging yet anyway.
		#should probably make a genbutton(id, coords[], sprites[], func(), "var", var, "label") thingy at this rate...
		Button = Window.CreateButton(96, xco[0], xco[1]+24, 24, 24)
		Button.SetSprites(bgfx[0],bgfx[1],bgfx[2],bgfx[3],bgfx[4],bgfx[5])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, PageGemRBOptionsWindow)
		Window.CreateLabel(95, xco[0]-38, xco[1]+24, 40, 24, fonts[1], "Next", IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE)
		Button.SetVarAssoc("GmFlag", GemOptionsPage+1)

		Button = Window.CreateButton(94, xco[0], xco[1]+48, 24, 24)
		Button.SetSprites(bgfx[0],bgfx[1],bgfx[2],bgfx[3],bgfx[4],bgfx[5])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, PageGemRBOptionsWindow)
		Window.CreateLabel(93, xco[0]-38, xco[1]+48, 40, 24, fonts[1], "Prev", IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE)
		if GemOptionsPage > 0:
			Button.SetVarAssoc("GmFlag", GemOptionsPage-1)

	RefreshGemRBOptionsWindow()
	Window.ShowModal(2) #isn't this supposed to black out other windows?

###################################################

#This is a helper debug readout for deciding what to do with the options:
def PrintOptions():
	print "GUIOPT: Panel will be generated for: " + str(len(OptionGroups)) + " Option Groups"

	for g in range(len(OptionGroups)):
		group = OptionGroups[g]
		for key in group:
			option = str(group[key])
			if option.isdigit(): #this can only mean a guiflag instead of ini key
				print "GUIEnh Bit " + option + " == " + str(GemRB.GetVar("GUIEnhancements")&int(option)) + " (" + key + ")"
			else:
				print "Option " + option + " == " + str(GemRB.GetVar(option)) + " (" + key + ")"
				if option in SpecialValues:
					print "Option " + option + " has a maximum value of " + str(SpecialValues[option])
				if option+"Min" in SpecialValues:
					print "Option " + option + " has a minimum value of " + str(SpecialValues[option+"Min"])
				if option+"Control" in SpecialValues:
					print "Option " + option + " would in an ideal world like to be set with a " + str(SpecialValues[option+"Control"])
					#etcetera

#it can be safely removed once the gemrb options screen is finished.
