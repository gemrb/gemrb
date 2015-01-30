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
	return DisplayTestResult (failures == 0, "TESTS", msg)[1]

def DisplayTestResult (status, name, intro=""):
	if status:
		msg = "%s:\t\t[color=00ff00]passed[/color]\n" %(name)
	else:
		msg = "%s:\t\t[color=ff0000]FAILURE![/color]\n" %(name)
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

# TextArea family of tests
def RunTextAreaTests():
	MessageWindow = GemRB.GetVar ("MessageWindow")
	MessageTA = GUIClasses.GTextArea (MessageWindow, GemRB.GetVar ("MessageTextArea"))

	tests = [ TA_AppendEmpty, TA_AppendSpace, TA_AppendNewline, TA_AppendNewlines, \
		TA_AppendNewlinesAtStart ]
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

def TA_AppendText(TA, name, text):
	old = TA.QueryText ()

	TA.Append (text)
	new = TA.QueryText ()

	status = (new == (old+text))
	if not status:
		print "Expected:+", repr(old+text), "+"
		print "Actual:+", repr(new), "+"
	return DisplayTestResult (status, name)

###################################################
# x family of tests
#
