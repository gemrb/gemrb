#ifndef 2DAIMP_H
#define 2DAIMP_H

#include "../Core/AnimationMgr.h"
#include "../../includes/globals.h"
#include "../../includes/RGBAColor.h"

typedef struct RowEntry {
	std::vector<char *> entries;
} RowEntry;

class p2DAImp : public TableMgr
{
private:
	DataStream * str;
	bool autoFree;
	std::vector<RowEntry> rows;
public:
	2DAImp(void);
	~2DAImp(void);
	bool Open(DataStream * stream, bool autoFree = false);
	inline int GetRowCount() { return rows.size(); }
	inline int GetColumnCount()
        {
        	if(rows.size()<=0) return 0;
		return rows[0].size();
        }
};

#endif
