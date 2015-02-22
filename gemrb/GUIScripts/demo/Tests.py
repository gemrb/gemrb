# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2015 The GemRB Project
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

import GemRB
import GUIClasses
import inspect

# all test groups return a tuple: (successes, failures, msg)
# all test cases return a tuple: (status, msg)

# runs individual test groups and keeps score
def RunTests():
	groups = [ RunTextAreaTests ]

	successes = 0
	failures = 0
	msg = "\n"
	for group in groups:
		results = group ()
		successes += results[0]
		failures += results[1]
		msg += results[2] + "\n"

	total = successes + failures
	msg += "TOTAL tests: %d\tsuccess: %d\tfailure: %d\n" %(total, successes, failures)
	return DisplayTestResult ("TESTS", failures, 0, msg)[1]

def DisplayTestResult (name, actual, expected, intro=""):
	status = (actual == expected)
	if status:
		msg = "%s:\t\t[color=00ff00]passed[/color]\n" %(name)
	else:
		msg = "%s:\t\t[color=ff0000]FAILURE![/color]\n" %(name)
		print "Expected:+", repr(expected), "+"
		print "Actual  :+", repr(actual), "+"

	return (status, intro+msg)

# returns function name
def Me():
	f = inspect.currentframe().f_back
	name = f.f_code.co_name
	# strip prefix
	return name.split("_")[1]

###################################################
# actual tests follow
#

# fits perfectly
fmline = "supercalifragilisticexpialidociousnessOFantidisestablishmentarianSesquipedalianists"

# TextArea family of tests
def RunTextAreaTests():
	MessageWindow = GemRB.GetVar ("MessageWindow")
	MessageTA = GUIClasses.GTextArea (MessageWindow, GemRB.GetVar ("MessageTextArea"))

	tests = [ TA_SetEmpty, TA_SetNone, TA_SetSpaces, TA_SetSupercali, TA_SetSupercali2 ]
	tests += [ TA_AppendEmpty, TA_AppendSpace, TA_AppendNewline, TA_AppendNewlines, \
		TA_AppendNewlinesAtStart, TA_AppendSupercali, TA_AppendSupercali2, \
		TA_AppendRef, TA_AppendRef2, TA_AppendRef3 ]
	tests += [ TA_PrependEmpty, TA_PrependSpace, TA_PrependNewline, TA_PrependNewlines, \
		TA_PrependTabs, TA_PrependTabsTag, TA_PrependTabsIncompleteTag ]
	msg = "Testing TextAreas\n"
	successes = 0
	failures = 0
	for test in tests:
		results = test (MessageTA)
		if results[0] == True:
			successes += 1
		else:
			failures += 1
		msg += results[1]
		MessageTA.SetText("") # reset

	return (successes, failures, msg)

def TA_AppendEmpty(TA):
	return TA_AppendText (TA, Me(), "")

def TA_AppendSpace(TA):
	return TA_AppendText (TA, Me(), " ")

def TA_AppendNewline(TA):
	return TA_AppendText (TA, Me(), "\n")

def TA_AppendNewlines(TA):
	return TA_AppendText (TA, Me(), "\n\n\n")

def TA_AppendNewlinesAtStart(TA):
	TA.Append ("---")
	return TA_AppendText (TA, Me(), "\n\nTEST")

def TA_AppendSupercali(TA):
	return TA_AppendText (TA, Me(), fmline)

def TA_AppendSupercali2(TA):
	TA.SetText (fmline[:-10])
	return TA_AppendText (TA, Me(), fmline * 3)

str1 = GemRB.GetString (1)
def TA_AppendRef(TA):
	return TA_AppendText (TA, Me(), 1, str1)

def TA_AppendRef2(TA):
	TA.SetText ("lalala")
	return TA_AppendText (TA, Me(), 1, "lalala"+str1)

def TA_AppendRef3(TA):
	TA.Append (1)
	TA.Append (': ')
	return TA_AppendText (TA, Me(), 1, str1+": "+str1)

def TA_AppendText(TA, name, text, expected=-1):
	old = TA.QueryText ()
	if expected == -1:
		expected = old+text

	TA.Append (text)
	new = TA.QueryText ()

	return DisplayTestResult (name, new, expected)

def TA_SetEmpty(TA):
	return TA_SetText (TA, Me(), "")

def TA_SetNone(TA):
	return TA_SetText (TA, Me(), None, "")

def TA_SetSpaces(TA):
	return TA_SetText (TA, Me(), "  \t\t  ")

def TA_SetSupercali(TA):
	return TA_SetText (TA, Me(), fmline)

def TA_SetSupercali2(TA):
	line = "[color=7b7a4e]" + fmline * 3 + "[/color]"
	return TA_SetText (TA, Me(), line)

def TA_SetText(TA, name, text, expected=-1):
	TA.SetText (text)
	new = TA.QueryText ()
	if expected == -1:
		expected = text

	return DisplayTestResult (name, new, expected)

def TA_PrependEmpty(TA):
	return TA_PrependText (TA, Me(), "")

def TA_PrependSpace(TA):
	return TA_PrependText (TA, Me(), " ")

def TA_PrependNewline(TA):
	return TA_PrependText (TA, Me(), "\n")

def TA_PrependNewlines(TA):
	return TA_PrependText (TA, Me(), "\n\n\n")

def TA_PrependTabs(TA):
	return TA_PrependText (TA, Me(), "\t\t")

def TA_PrependTabsIncompleteTag(TA):
	return TA_PrependText (TA, Me(), "\t\t", "[color=0000ff]")

def TA_PrependTabsTag(TA):
	return TA_PrependText (TA, Me(), "\t\t", "[color=0000ff]asd[/color]")

def TA_PrependText(TA, name, text, old=-1):
	if old == -1:
		old = TA.QueryText ()

	TA.SetText (text+old)
	new = TA.QueryText ()

	return DisplayTestResult (name, new, text+old)

###################################################
# x family of tests
#
