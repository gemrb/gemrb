# common character generation display code
import GemRB
from ie_stats import *
from GUIDefines import *
from GUICommon import *
from GUICommonWindows import GetKitIndex

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def DisplayOverview(step):
	global CharGenWindow, TextAreaControl, PortraitName

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindowObject (0)
	CharGenWindow.SetFrame ()
	GemRB.SetVar ("Step", step)

	###
	# Buttons
	###
	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	if PortraitName != "":
		PortraitButton.SetPicture (PortraitName, "NOPORTMD")
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	GenderButton = CharGenWindow.GetControl (0)
	GenderButton.SetText (11956)
	SetButtonStateFromStep ("GenderButton", GenderButton, step)

	RaceButton = CharGenWindow.GetControl (1)
	RaceButton.SetText (11957)
	SetButtonStateFromStep ("RaceButton", RaceButton, step)

	ClassButton = CharGenWindow.GetControl (2)
	ClassButton.SetText (11959)
	SetButtonStateFromStep ("ClassButton", ClassButton, step)

	AlignmentButton = CharGenWindow.GetControl (3)
	AlignmentButton.SetText (11958)
	SetButtonStateFromStep ("AlignmentButton", AlignmentButton, step)

	AbilitiesButton = CharGenWindow.GetControl (4)
	AbilitiesButton.SetText (11960)
	SetButtonStateFromStep ("AbilitiesButton", AbilitiesButton, step)

	SkillButton = CharGenWindow.GetControl (5)
	SkillButton.SetText (17372)
	SetButtonStateFromStep ("SkillButton", SkillButton, step)

	AppearanceButton = CharGenWindow.GetControl (6)
	AppearanceButton.SetText (11961)
	SetButtonStateFromStep ("AppearanceButton", AppearanceButton, step)

	NameButton = CharGenWindow.GetControl (7)
	NameButton.SetText (11963)
	SetButtonStateFromStep ("NameButton", NameButton, step)

	BackButton = CharGenWindow.GetControl (11)
	BackButton.SetText (15416)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "BackPress")
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AcceptButton = CharGenWindow.GetControl (8)
	playmode = GemRB.GetVar ("PlayMode")
	if playmode>=0:
		AcceptButton.SetText (11962)
	else:
		AcceptButton.SetText (13956)
	SetButtonStateFromStep ("AcceptButton", AcceptButton, step)
	#AcceptButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	#AcceptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "NextPress")

	ScrollBar = CharGenWindow.GetControl (10)
	ScrollBar.SetDefaultScrollBar ()

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetText (13955)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ImportPress")

	CancelButton = CharGenWindow.GetControl (15)
	if step == 1:
		CancelButton.SetText (13727) # Cancel
	else:
		CancelButton.SetText (8159) # Start over
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CancelPress")

	BiographyButton = CharGenWindow.GetControl (16)
	BiographyButton.SetText (18003)
	BiographyButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BiographyPress")
	if step == 9:
		BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	###
	# Stat overview
	###
	RaceTable = GemRB.LoadTableObject ("races")
	ClassTable = GemRB.LoadTableObject ("classes")
	KitTable = GemRB.LoadTableObject ("kitlist")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	AbilityTable = GemRB.LoadTableObject ("ability")

	MyChar = GemRB.GetVar ("Slot")

	for part in range(1, step+1):
		if part == 1:
			TextAreaControl= CharGenWindow.GetControl (9)
			if step == 1:
				TextAreaControl.SetText ("[capital=0]" + GemRB.GetString(16575))
			elif step == 9:
				TextAreaControl.SetText ("[capital=0]" + GemRB.GetString(1047))
			else:
				TextAreaControl.SetText ("[capital=0]" + GemRB.GetString(12135))
		elif part == 2:
			TextAreaControl.Append (": ")
			if step == 9:
				TextAreaControl.Append (GemRB.GetToken ("CHARNAME") )
				TextAreaControl.Append (12135, -1)
				TextAreaControl.Append (": ")
			if GemRB.GetVar ("Gender") == 1:
				TextAreaControl.Append (1050)
			else:
				TextAreaControl.Append (1051)
		elif part == 3:
			TextAreaControl.Append (1048, -1) # new line
			TextAreaControl.Append (": ")
			TextAreaControl.Append (RaceTable.GetValue (GemRB.GetVar ("Race")-1,2))
		elif part == 4:
			TextAreaControl.Append (12136, -1)
			TextAreaControl.Append (": ")
			KitIndex = GemRB.GetVar ("Class Kit")
			if KitIndex == 0: # FIXME: use GetPCTitle from GUIREC?
				Class = GemRB.GetVar ("Class")-1
				ClassTitle=ClassTable.GetValue (Class, 2)
			else:
				ClassTitle=KitTable.GetValue (KitIndex, 2)
			TextAreaControl.Append (ClassTitle)
		elif part == 5:
			TextAreaControl.Append (1049, -1)
			TextAreaControl.Append (": ")
			v = AlignmentTable.FindValue (3, GemRB.GetVar ("Alignment"))
			TextAreaControl.Append (AlignmentTable.GetValue (v,2))
		elif part == 6:
			TextAreaControl.Append ("\n")
			for i in range(6):
				v = AbilityTable.GetValue (i, 2)
				TextAreaControl.Append (v, -1)
				TextAreaControl.Append (": " + str(GemRB.GetVar ("Ability "+str(i))) )
		elif part == 7:
			TextAreaControl.Append ("\n\n")
			# thieving and other skills
			info = ""
			SkillTable = GemRB.LoadTableObject ("skills")
			KitList = GemRB.LoadTableObject ("kitlist")
			ClassTable = GemRB.LoadTableObject ("classes")
			Class = GemRB.GetVar ("Class") - 1
			ClassID = ClassTable.GetValue (Class, 5)
			Class = ClassTable.FindValue (5, ClassID)
			# TODO: what about monks and any others not in skills.2da?
			# TODO also before: skill{rng,brd}.2da <-- add rangers to clskills.2da
			KitName = GetKitIndex (MyChar)
			if KitName == 0:
				KitName = ClassTable.GetRowName (Class)
			else:
				KitName = KitList.GetValue (KitName, 0)

			if SkillTable.GetValue ("RATE", KitName) != -1:
				for skill in range(SkillTable.GetRowCount () - 2):
					name = SkillTable.GetValue (skill+2, 1)
					name = GemRB.GetString (name)
					value = GemRB.GetVar ("Skill " + str(skill))
					if value >= 0:
						info += name + ": " + str(value) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				TextAreaControl.Append (8442)
				TextAreaControl.Append (info)

			# arcane spells
			info = ""
			for level in range(0, 9):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_WIZARD, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_WIZARD, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'])['SpellName']
					info += GemRB.GetString (Spell) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				TextAreaControl.Append (11027)
				TextAreaControl.Append (info)

			# divine spells
			info = ""
			for level in range(0, 7):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_PRIEST, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'])['SpellName']
					info += GemRB.GetString (Spell) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				TextAreaControl.Append (11028)
				TextAreaControl.Append (info)

			# racial enemy
			info = ""
			Race = GemRB.GetVar ("HateRace")
			if Race:
				RaceTable = GemRB.LoadTableObject ("HATERACE")
				Row = RaceTable.FindValue (1, Race)
				info = GemRB.GetString (RaceTable.GetValue(Row, 0))
				if info != "":
					info = "\n" + info + "\n\n"
					TextAreaControl.Append (15982)
					TextAreaControl.Append (info)

			# weapon proficiencies
			TextAreaControl.Append (9466)
			TextAreaControl.Append ("\n")
			TmpTable=GemRB.LoadTableObject ("weapprof")
			ProfCount = TmpTable.GetRowCount ()
			#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
			for i in range(ProfCount-8):
				# 4294967296 overflows to -1 on some arches, so we use a smaller invalid strref
				strref = TmpTable.GetValue (i+8, 1)
				if strref == -1 or strref > 500000:
					continue
				Weapon = GemRB.GetString (TmpTable.GetValue (i+8, 1))
				Value = GemRB.GetVar ("Prof "+str(i) )
				if Value:
					pluses = " "
					for plus in range(0, Value):
						pluses += "+"
					TextAreaControl.Append (Weapon + pluses + "\n")

		elif part == 8:
			break

	CharGenWindow.SetVisible (1)
	return

def SetButtonStateFromStep (buttonName, button, step):
	global CharGenWindow

	state = IE_GUI_BUTTON_DISABLED
	if buttonName == "GenderButton":
		if step == 1:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "RaceButton":
		if step == 2:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "ClassButton":
		if step == 3:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AlignmentButton":
		if step == 4:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AbilitiesButton":
		if step == 5:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "SkillButton":
		if step == 6:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AppearanceButton":
		if step == 7:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "NameButton":
		if step == 8:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AcceptButton":
		if step == 9:
			state = IE_GUI_BUTTON_ENABLED
	button.SetState (state)

	if state == IE_GUI_BUTTON_ENABLED:
		button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
		button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "NextPress")
	return

def CancelPress():
	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
		#free up the slot before exiting
		MyChar = GemRB.GetVar ("Slot")
		GemRB.CreatePlayer ("", MyChar | 0x8000 )
		GemRB.SetNextScript ("Start")
	else:
		GemRB.SetNextScript ("CharGen")
	return

def ImportPress():
	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	# TODO: check why this is handled differently
	if step == 1:
		GemRB.SetNextScript("GUICG24")
	else:
		GemRB.SetToken ("NextScript", "CharGen9")
		GemRB.SetNextScript ("ImportFile") #import
	return

def BackPress():
	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
		GemRB.SetNextScript ("Start")
	elif step == 2:
		GemRB.SetNextScript ("CharGen")
	else:
		GemRB.SetNextScript ("CharGen" + str(step-1))
	return

def NextPress():
	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
		GemRB.SetNextScript ("GUICG1")
	elif step == 2:
		GemRB.SetNextScript ("GUICG8")
	elif step == 6:
		GemRB.SetNextScript ("GUICG15")
	elif step == 7:
		GemRB.SetNextScript ("GUICG13")
	elif step == 8:
		GemRB.SetNextScript ("GUICG5")
	elif step == 9:
		pass #FinishCharGen()
	else: # 3, 4, 5
		GemRB.SetNextScript ("GUICG" + str(step-1))
	return

def BiographyPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("GUICG23") #biography
	return
