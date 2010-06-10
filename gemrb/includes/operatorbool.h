#ifndef OPERATORBOOL_H
#define OPERATORBOOL_H
// Copied from boost/smart_ptr/detail/operator_bool.hpp

#ifndef _MSC_VER
#define OPERATOR_BOOL(Class,T,ptr) operator T* Class::*() const { return ptr == NULL ? NULL : &Class::ptr; }
#else // MSVC6
	// FIXME: Figure out what version doesn't need this hack.
#define OPERATOR_BOOL(Class,T,ptr) operator bool() const { return ptr != NULL; }
#endif

#endif
