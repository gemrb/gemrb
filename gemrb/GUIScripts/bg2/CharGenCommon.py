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
	CharGenWindow = GemRB.LoadWindow (0)
	GemRB.SetWindowFrame (CharGenWindow)
	GemRB.SetVar ("Step", step)

	###
	# Buttons
	###
	PortraitButton = GemRB.GetControl (CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	if PortraitName != "":
		GemRB.SetButtonPicture (CharGenWindow, PortraitButton, PortraitName, "NOPORTMD")
	GemRB.SetButtonState (CharGenWindow, PortraitButton, IE_GUI_BUTTON_LOCKED)

	GenderButton = GemRB.GetControl (CharGenWindow, 0)
	GemRB.SetText (CharGenWindow, GenderButton, 11956)
	SetButtonStateFromStep ("GenderButton", GenderButton, step)

	RaceButton = GemRB.GetControl (CharGenWindow, 1)
	GemRB.SetText (CharGenWindow, RaceButton, 11957)
	SetButtonStateFromStep ("RaceButton", RaceButton, step)

	ClassButton = GemRB.GetControl (CharGenWindow, 2)
	GemRB.SetText (CharGenWindow, ClassButton, 11959)
	SetButtonStateFromStep ("ClassButton", ClassButton, step)

	AlignmentButton = GemRB.GetControl (CharGenWindow, 3)
	GemRB.SetText (CharGenWindow, AlignmentButton, 11958)
	SetButtonStateFromStep ("AlignmentButton", AlignmentButton, step)

	AbilitiesButton = GemRB.GetControl (CharGenWindow, 4)
	GemRB.SetText (CharGenWindow, AbilitiesButton, 11960)
	SetButtonStateFromStep ("AbilitiesButton", AbilitiesButton, step)

	SkillButton = GemRB.GetControl (CharGenWindow, 5)
	GemRB.SetText (CharGenWindow,SkillButton, 17372)
	SetButtonStateFromStep ("SkillButton", SkillButton, step)

	AppearanceButton = GemRB.GetControl (CharGenWindow, 6)
	GemRB.SetText (CharGenWindow,AppearanceButton, 11961)
	SetButtonStateFromStep ("AppearanceButton", AppearanceButton, step)

	NameButton = GemRB.GetControl (CharGenWindow, 7)
	GemRB.SetText (CharGenWindow, NameButton, 11963)
	SetButtonStateFromStep ("NameButton", NameButton, step)

	BackButton = GemRB.GetControl (CharGenWindow, 11)
	GemRB.SetText (CharGenWindow, BackButton, 15416)
	GemRB.SetButtonState (CharGenWindow,BackButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent (CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")

	AcceptButton = GemRB.GetControl (CharGenWindow, 8)
	playmode = GemRB.GetVar ("PlayMode")
	if playmode>=0:
		GemRB.SetText (CharGenWindow, AcceptButton, 11962)
	else:
		GemRB.SetText (CharGenWindow, AcceptButton, 13956)
	SetButtonStateFromStep ("AcceptButton", AcceptButton, step)
	GemRB.SetButtonFlags(CharGenWindow,AcceptButton, IE_GUI_BUTTON_DEFAULT,OP_OR)
	GemRB.SetEvent (CharGenWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")

	ImportButton = GemRB.GetControl (CharGenWindow, 13)
	GemRB.SetText (CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState (CharGenWindow,ImportButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent (CharGenWindow, ImportButton, IE_GUI_BUTTON_ON_PRESS, "ImportPress")

	CancelButton = GemRB.GetControl (CharGenWindow, 15)
	if step == 1:
		GemRB.SetText (CharGenWindow, CancelButton, 13727) # Cancel
	else:
		GemRB.SetText (CharGenWindow, CancelButton, 8159) # Start over
	GemRB.SetButtonState (CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent (CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")

	BiographyButton = GemRB.GetControl (CharGenWindow, 16)
	GemRB.SetText (CharGenWindow, BiographyButton, 18003)
	GemRB.SetEvent(CharGenWindow, BiographyButton, IE_GUI_BUTTON_ON_PRESS, "BiographyPress")
	if step == 9:
		GemRB.SetButtonState (CharGenWindow, BiographyButton, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (CharGenWindow, BiographyButton, IE_GUI_BUTTON_DISABLED)

	###
	# Stat overview
	###
	RaceTable = GemRB.LoadTable ("races")
	ClassTable = GemRB.LoadTable ("classes")
	KitTable = GemRB.LoadTable ("kitlist")
	AlignmentTable = GemRB.LoadTable ("aligns")
	AbilityTable = GemRB.LoadTable ("ability")

	MyChar = GemRB.GetVar ("Slot")

	for part in range(1, step+1):
		if part == 1:
			TextAreaControl= GemRB.GetControl (CharGenWindow,9)
			if step == 1:
				GemRB.SetText (CharGenWindow, TextAreaControl, "[capital=0]" + GemRB.GetString(16575))
			elif step == 9:
				GemRB.SetText (CharGenWindow, TextAreaControl, "[capital=0]" + GemRB.GetString(1047))
			else:
				GemRB.SetText (CharGenWindow, TextAreaControl, "[capital=0]" + GemRB.GetString(12135))
		elif part == 2:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": ")
			if step == 9:
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, GemRB.GetToken ("CHARNAME") )
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 12135, -1)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
			if GemRB.GetVar ("Gender") == 1:
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1050)
			else:
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1051)
		elif part == 3:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1048, -1) # new line
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": ")
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, \
				GemRB.GetTableValue (RaceTable, GemRB.GetVar ("Race")-1,2))
		elif part == 4:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 12136, -1)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
			KitIndex = GemRB.GetVar ("Class Kit")
			if KitIndex == 0: # FIXME: use GetPCTitle from GUIREC?
				Class = GemRB.GetVar ("Class")-1
				ClassTitle=GemRB.GetTableValue (ClassTable, Class, 2)
			else:
				ClassTitle=GemRB.GetTableValue (KitTable, KitIndex, 2)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ClassTitle)
		elif part == 5:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1049, -1)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": ")
			v = GemRB.FindTableValue (AlignmentTable, 3, GemRB.GetVar ("Alignment"))
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,GemRB.GetTableValue (AlignmentTable,v,2))
		elif part == 6:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, "\n")
			for i in range(6):
				v = GemRB.GetTableValue (AbilityTable, i, 2)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, v, -1)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": " \
					+ str(GemRB.GetVar ("Ability "+str(i))) )
		elif part == 7:
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, "\n\n")
			# thieving and other skills
			info = ""
			SkillTable = GemRB.LoadTable ("skills")
			KitList = GemRB.LoadTable ("kitlist")
			ClassTable = GemRB.LoadTable ("classes")
			Class = GemRB.GetVar ("Class") - 1
			ClassID = GemRB.GetTableValue (ClassTable, Class, 5)
			Class = GemRB.FindTableValue (ClassTable, 5, ClassID)
			# TODO: what about monks and any others not in skills.2da?
			# TODO also before: skill{rng,brd}.2da <-- add rangers to clskills.2da
			KitName = GetKitIndex (MyChar)
			if KitName == 0:
				KitName = GemRB.GetTableRowName (ClassTable, Class)
			else:
				KitName = GemRB.GetTableValue (KitList, KitName, 0)

			if GemRB.GetTableValue (SkillTable,"RATE", KitName) != -1:
				for skill in range(GemRB.GetTableRowCount (SkillTable) - 2):
					name = GemRB.GetTableValue (SkillTable, skill+2, 1)
					name = GemRB.GetString (name)
					value = GemRB.GetVar ("Skill " + str(skill))
					if value >= 0:
						info += name + ": " + str(value) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 8442)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, info)

			# arcane spells
			info = ""
			for level in range(0, 9):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_WIZARD, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_WIZARD, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'])['SpellName']
					info += GemRB.GetString (Spell) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 11027)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, info)

			# divine spells
			info = ""
			for level in range(0, 7):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_PRIEST, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'])['SpellName']
					info += GemRB.GetString (Spell) + "\n"
			if info != "":
				info = "\n" + info + "\n"
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 11028)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, info)

			# racial enemy
			Race = GemRB.GetVar ("HateRace")
			RaceTable = GemRB.LoadTable ("HATERACE")
			Row = GemRB.FindTableValue (RaceTable, 1, Race)
			info = GemRB.GetString (GemRB.GetTableValue(RaceTable, Row, 0))
			if info != "":
				info = "\n" + info + "\n\n"
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 15982)
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, info)

			# weapon proficiencies
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 9466)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, "\n")
			TmpTable=GemRB.LoadTable ("weapprof")
			ProfCount = GemRB.GetTableRowCount (TmpTable)
			#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
			for i in range(ProfCount-8):
				Weapon = GemRB.GetString (GemRB.GetTableValue (TmpTable, i+8, 1))
				Value = GemRB.GetVar ("Prof "+str(i) )
				if Value:
					pluses = " "
					for plus in range(0, Value):
						pluses += "+"
					GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, Weapon + pluses + "\n")
			GemRB.UnloadTable (TmpTable)

		elif part == 8:
			break

	GemRB.SetVisible (CharGenWindow, 1)
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
	GemRB.SetButtonState (CharGenWindow, button, state)

	if state == IE_GUI_BUTTON_ENABLED:
		GemRB.SetButtonFlags (CharGenWindow, button, IE_GUI_BUTTON_DEFAULT, OP_OR)
		GemRB.SetEvent (CharGenWindow, button, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	return

def CancelPress():
	global CharGenWindow
	GemRB.UnloadWindow (CharGenWindow)

	step = GemRB.GetVar ("Step")
	if step == 1:
		GemRB.SetNextScript ("Start")
	else:
		GemRB.SetNextScript ("CharGen")
	return

def ImportPress():
	global CharGenWindow
	GemRB.UnloadWindow (CharGenWindow)

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
	GemRB.UnloadWindow (CharGenWindow)

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
	GemRB.UnloadWindow (CharGenWindow)

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
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG23") #biography
	return
