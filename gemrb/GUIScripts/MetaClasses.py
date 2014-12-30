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


# The metaclasses below are used to define objects that call
# functions in the GemRB module.
#
# Example:
# class GTable:
#  __metaclass__ = metaIDWrapper
#  methods = {
#    'GetValue': GemRB.GetTableValue,
#  }
#
# x = GTable(5)
#
# Calling
# x.GetValue("Row", "Col")
# will then execute
# GemRB.GetTableValue(5, "Row", "Col")

def make_caller_lambda_ID(M):
  return lambda self, *args: M(self.ID, *args)
class metaIDWrapper(type):
  def __new__(cls, classname, bases, classdict):
    def __init__(self, ID):
      self.ID = ID
    newdict = { '__slots__':['ID'], '__init__':__init__, }
    if len(bases) == 1:
      def __subinit__(self, ID):
        bases[0].__init__(self, ID)
      newdict['__init__'] = __subinit__
      newdict['__slots__'] = []
    methods = classdict['methods']
    for key in methods: 
      newdict[key] = make_caller_lambda_ID(methods[key])
    for key in classdict:
      if key != 'methods':
        newdict[key] = classdict[key]
    return type.__new__(cls, classname, bases, newdict)


# metaControl has two extra arguments: WinID and ID
def make_caller_lambda_Control(M):
  return lambda self, *args: M(self.WinID, self.ID, *args)
class metaControl(type):
  def __new__(cls, classname, bases, classdict):
    def __init__(self, WinID, ID):
      self.WinID = WinID
      self.ID = ID
   
    if len(bases) == 1:
      def __subinit__(self, WinID, ID):
        bases[0].__init__(self, WinID, ID)
      init = __subinit__
      slots = []
    else:
      slots = ['WinID', 'ID']
      init = __init__
 
    slots.extend(classdict.pop('assignableAttributes', []))
    newdict = { '__slots__' : slots, '__init__' : init, }

    methods = classdict.pop('methods', {})
    for key in methods:
      newdict[key] = make_caller_lambda_Control(methods[key])
    
    # we've poped everything we dont want, just insert the rest of classdict
    # FIXME?: we are still able to overwrite things like the "__slots__" entry
    newdict.update(classdict)
    return type.__new__(cls, classname, bases, newdict)

