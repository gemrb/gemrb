#include "../../includes/win32def.h"
#include "2DAImp.h"
#include "../Core/FileStream.h"

p2DAImp::p2DAImp(void)
{
	str = NULL;
	autoFree = false;
}

p2DAImp::~p2DAImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool p2DAImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	DataStream * olds = str;
	str = stream;
	char Signature[8];
	str->CheckEncrypted();
	if(strncmp(Signature, "2DA V1.0", 8) != 0) {
		str = olds;
		return false;
	}
	return true;
}
