#ifndef IDSIMP_H
#define IDSIMP_H

#include "../Core/SymbolMgr.h"

typedef struct Pair {
	int val;
	char * str;
} Pair;

class IDSImp : public SymbolMgr
{
private:
	DataStream * str;
	bool autoFree;

	std::vector<Pair> pairs;
	std::vector<char*> ptrs;

public:
	IDSImp(void);
	~IDSImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	long GetValue(const char * txt);
	const char * GetValue(int val);
public:
	void release(void)
	{
		delete this;
	}
};

#endif

