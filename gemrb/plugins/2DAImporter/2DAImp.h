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
	std::vector<char *> colNames;
	std::vector<char *> rowNames;
	std::vector<char *> ptrs;
	std::vector<RowEntry> rows;
	char defVal[32];
public:
	p2DAImp(void);
	~p2DAImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	/** Returns the actual number of Rows in the Table */
	inline int GetRowCount() { return rows.size(); }
	/** Returns the actual number of Columns in the Table */
	inline int GetColumnCount()
        {
        	if(rows.size()<=0) return 0;
			return rows[0].size();
        }
	/** Returns a pointer to a zero terminated 2da element,
        0,0 returns the default value, it may return NULL */
    inline char *QueryField(unsigned int row = 0, unsigned int column = 0) const
	{
		if(rows.size()<=row) return (char*)defVal;
		if(rows[row].size()<=column) return (char*)defVal;
		return rows[row][column];
	};
	/** Returns a pointer to a zero terminated 2da element,
      uses column name and row name to search the field,
	  may return NULL */
	inline char *QueryField(const char* row, const char* column) const
	{
		int rowi = -1, coli = -1;
		for(int i = 0; i < rowNames.size(); i++) {
			if(stricmp(rowNames[i], row) == 0) {
				rowi = i;
				break;
			}
		}
		if(rowi == -1)
			return (char*)defVal;
		for(int i = 0; i < colNames.size(); i++) {
			if(stricmp(colNames[i], column) == 0) {
				coli = i;
				break;
			}
		}
		if(coli == -1)
			return (char*)defVal;
		if(rows[rowi].size() <= coli)
			return (char*)defVal;
		return rows[rowi][coli];
	};

	inline int GetRowIndex(const char *string) const
	{
		for(int index = 0; index<rowNames.size(); index++) {
			if(stricmp(rowNames[index],string) == 0) {
				return index;
			}
		}
		return -1;
	};

	inline char *GetRowName(unsigned int index) const
	{
		if(index < rowNames.size())
			return rowNames[index];
		return NULL;
	};
public:
	void release(void)
	{
		delete this;
	}
};

#endif
