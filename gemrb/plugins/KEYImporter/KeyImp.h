#ifndef KEYIMP_H
#define KEYIMP_H

#include <vector>
#include "../Core/ResourceMgr.h"

typedef struct RESEntry {
	char			ResRef[8];
	unsigned short	Type;
	unsigned long	ResLocator;
} RESEntry;

typedef struct BIFEntry {
	char * name;
	unsigned short BIFLocator;
} BIFEntry;

class KeyImp : public ResourceMgr
{
private:
	std::vector<BIFEntry> biffiles;
	std::vector<RESEntry> resources;
public:
	KeyImp(void);
	~KeyImp(void);
	bool LoadResFile(const char * resfile);
	DataStream * GetResource(const char * resname, SClass_ID type);
	void * GetFactoryResource(const char * resname, SClass_ID type, unsigned char mode = IE_NORMAL);
};

#endif
