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

from types import MethodType

def add_metaclass(metaclass):
    """Class decorator for creating a class with a metaclass."""
    def wrapper(cls):
        orig_vars = cls.__dict__.copy()
        slots = orig_vars.get('__slots__')
        if slots is not None:
            if isinstance(slots, str):
                slots = [slots]
            for slots_var in slots:
                orig_vars.pop(slots_var, None)
        orig_vars.pop('__dict__', None)
        orig_vars.pop('__weakref__', None)
        if hasattr(cls, '__qualname__'):
            orig_vars['__qualname__'] = cls.__qualname__
        return metaclass(cls.__name__, cls.__bases__, orig_vars)
    return wrapper

def MethodAttributeError(f):
	def handler(*args, **kwargs):
		try:
			return f(*args, **kwargs)
		except Exception as e:
			raise type(e)(str(e) + "\nMethod Docs:\n" + str(f.__doc__))
	return handler

class metaIDWrapper(type):		
	@classmethod
	def InitMethod(cls, f = None):
		def __init__(self, *args, **kwargs):
			for k,v in kwargs.items():
				setattr(self, k, v)

			#required attributes for bridging to C++
			assert getattr(self, 'ID', None) is not None

			if f:
				f(self, *args)
		return __init__
	
	def __new__(cls, classname, bases, classdict):
		classdict['__slots__'] = classdict.get('__slots__', [])
		classdict['__slots__'].append('ID')
		
		classdict['__init__'] = classdict.get('__init__', None)
		classdict['__init__'] = cls.InitMethod(classdict['__init__'])

		methods = classdict.pop('methods', {})
		c = super(metaIDWrapper, cls).__new__(cls, classname, bases, classdict)
		# we must bind the methods after the class is created (instead of adding to classdict)
		# otherwise the methods would be class methods instead of instance methods
		for key in methods:
			e = MethodAttributeError(methods[key])
			try:
				mtype = MethodType(e, None, c)
				setattr(c, key, mtype)
			except TypeError: # Python 3
				setattr(c, key, e) # FIXME?: I dont actually know if this is correct, may have to circle back here after overcoming other errors
		return c
