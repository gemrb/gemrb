#include "../../includes/win32def.h"
#include "Class_ID.h"

Class_ID::Class_ID(void)
{
	pa = 0xFFFFFFFF;
	pb = 0xFFFFFFFF;
}

Class_ID::Class_ID(unsigned long aa, unsigned long bb)
{
	pa = aa;
	pb = bb;
}

Class_ID::Class_ID(Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();
}

Class_ID::Class_ID(const Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();
}

unsigned long Class_ID::PartA(void) const
{
	return pa;
}

unsigned long Class_ID::PartB(void) const
{
	return pb;
}

int Class_ID::operator==(const Class_ID& cid) const
{
	return ((pa == cid.PartA()) && (pb == cid.PartB()));
}

int Class_ID::operator!=(const Class_ID& cid) const
{
	return ((pa != cid.PartA()) || (pb == cid.PartB()));
}

Class_ID& Class_ID::operator=(const Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();	
	return *this;
}

bool Class_ID::operator<(const Class_ID& rhs) const
{
	return ((pa < rhs.PartA()) && (pb < rhs.PartB()));
}
