#ifndef CLASSID_H
#define CLASSID_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Class_ID
{
public:
	Class_ID(void);
	Class_ID(unsigned long aa, unsigned long bb);
	Class_ID(Class_ID& cid);
	Class_ID(const Class_ID& cid);
	unsigned long PartA(void) const;
	unsigned long PartB(void) const;
	int operator==(const Class_ID& cid) const;
	int operator!=(const Class_ID& cid) const;
	Class_ID& operator=(const Class_ID& cid);
	bool operator<(const Class_ID& rhs) const;
private:
	unsigned long pa, pb;
};

#endif
