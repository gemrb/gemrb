#ifndef IDSIMP_H
#define IDSIMP_H

#include "../Core/SymbolMgr.h"

class IDSImp : public SymbolMgr
{
private:
	DataStream * str;
	bool autoFree;

	//Data
	char ** text;
	long * value;
	unsigned long arraySize;

	int ReadLine(char * lineBuf);
	int CheckHeader(const char *lineBuf);
	bool IsNumeric(char * checkString, bool isUnsigned = false);
	bool IsHex(char * checkString);
	void ClearArrays(void);
	bool ResizeArrays(int x, unsigned long intHeader);

public:
	IDSImp(void);
	~IDSImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	long GetValue(char * txt);
};

#endif

