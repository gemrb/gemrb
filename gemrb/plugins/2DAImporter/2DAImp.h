#ifndef P2DAIMP_H
#define P2DAIMP_H

#include "../Core/TableMgr.h"
#include "../../includes/globals.h"

typedef std::vector<char *> RowEntry;

class p2DAImp : public TableMgr
{
private:
	DataStream * str;
	bool autoFree;
	std::vector<RowEntry> rows;
public:
	p2DAImp(void);
	~p2DAImp(void);
	bool Open(DataStream * stream, bool autoFree = false);
	inline int GetRowCount() { return rows.size(); }
	inline int GetColumnCount()
        {
        	if(rows.size()<=0) return 0;
		return rows[0].size();
        }
        inline char *QueryField(int row = 0, int column = 0) const
	{
		if(rows.size()<=row) return NULL;
		if(rows[row].size()<=column) return NULL;
		return rows[row][column];
	};
};

#endif
