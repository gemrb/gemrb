#include "../../includes/win32def.h"
#include "IDSImp.h"
#include "IDSImpDefs.h"
#include <ctype.h>

IDSImp::IDSImp(void)
{
	str = NULL;
	text = NULL;
	value = NULL;
	arraySize = 0;
	autoFree = false;
}

IDSImp::~IDSImp(void)
{

	if(str && autoFree)
		delete(str);

	ClearArrays();
}

void IDSImp::ClearArrays(void)
{
	if(text)
	{
		for (unsigned int i = 0; i < (arraySize > MAX_LINES ? arraySize : MAX_LINES); i++)
			delete [] text[i];

		delete [] text;
	}

	if(value)
		delete [] value;
}

bool IDSImp::Open(DataStream * stream, bool autoFree)
{
	unsigned long intHeader = 0;
	int lineCount = 0;
	char lineBuf[MAX_LINE_LENGTH];
	bool retEof = false;

	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;


	ReadLine(lineBuf);
	lineCount++;

	switch ( CheckHeader(lineBuf) ) // check IDS file header
	{
		case HEADER_IDS:
			intHeader = MAX_TEXT_LENGTH;
			break;

		case HEADER_LENGTH:
			intHeader = strtoul(lineBuf, NULL, 10);
			break;

		case HEADER_BLANK:
			intHeader = MAX_TEXT_LENGTH;
			break;

		case HEADER_RECORD:
			str->Seek(0, SEEK_SET);
			intHeader = MAX_TEXT_LENGTH;
			break;

		case HEADER_ERROR:
			fprintf(stderr, "Error: Invalid IDS file encountered.\n");
			return false;
			break;

		default:
			fprintf(stderr, "Someone's been messing with the functions! :-) \n");
			break;
	}

	ClearArrays();

	value = new long [MAX_LINES];

	text = new char * [MAX_LINES];

	for (int i = 0; i < MAX_LINES; i++)
	{
		text[i] = new char [intHeader+1];
	}

	int x = 0;

	while (retEof == false)
	{
		int j = 0;
		int intBase = 10;
		char strVal[MAX_VALUE_LENGTH] = "";
		char * strTxt;

		strTxt = new char [intHeader+1];
		memset(strTxt, '\0', intHeader + 1);

		if ( ReadLine(lineBuf) == GEM_EOF )
			retEof = true;

		lineCount++;
		// split entry here
		sscanf(lineBuf, "%s %s", strVal, strTxt);

		if ( !strlen(strTxt) )
		{
			delete [] strTxt;
			continue;
		}
		else
			strncpy( text[x], strTxt, intHeader+1 ) ;

		delete [] strTxt;

		while (strVal[j] != '\0')
		{
			if (toupper(strVal[j]) == 'X') // if value is in hex
			{
				intBase = 16;
				break;
			}
			j++;
		}
		
		value[x] = strtoul(strVal, NULL, intBase);
		x++;

		if ( x >= MAX_LINES )
		{ /* Increase text and value array sizes if needed */
			ResizeArrays(x, intHeader);
		}
		
	}
	arraySize = x;

	return true;

}

bool IDSImp::ResizeArrays(int x, unsigned long intHeader)
{
	long * tmpVal;
	char ** tmpText;

	if ( (tmpVal = new long [x + 1]) == 0 )
		return false;
		
	if ( (tmpText = new char * [x + 1]) == 0 )
		return false;

	memcpy(tmpVal, value, x * sizeof(long) );
	memcpy(tmpText, text, x * sizeof(char *) );

	delete [] value;
	delete [] text;

	value = tmpVal;
	text = tmpText;

	text[x] = new char [intHeader + 1];

	return true;
}

long IDSImp::GetValue(char * txt)
{
	for (unsigned int i = 0; i < arraySize; i++)
	{
		if ( !strcmp(txt, text[i]) )
			return value[i];
	}
	return SYMBOL_VALUE_NOT_LOCATED;

}

int IDSImp::ReadLine(char * lineBuf)
{ // read a line from DataStream str into lineBuf

	char curChar;
	int i = 0;
	bool eofFound = false;

	if (str == NULL)
	{
		fprintf(stderr, "Stream isn't open\n");
		return GEM_ERROR;
	}

	memset(lineBuf, '\0', MAX_LINE_LENGTH);

	while ( i < MAX_LINE_LENGTH )
	{
		int ret = str->Read( &curChar, 1 );

		if ( ret == GEM_EOF )
		{
			eofFound = true;
			break; 
		}
		else if ( ret == GEM_ERROR )
			return ret;

		//if ( curChar == '\x0a' )
		if ( curChar == '\n')
			break;

		lineBuf[i++] = curChar;
	}

	lineBuf[i] = '\0';

	if ( eofFound == true )
		return GEM_EOF;

	return GEM_OK;

}

int IDSImp::CheckHeader(const char * lineBuf)
{
	char header1[MAX_HEADER_LENGTH] = "";
	char header2[MAX_HEADER_LENGTH] = "";

	sscanf(lineBuf, "%s %s", header1, header2);

	if ( !strncmp(header1, "IDS", 3) )
		return HEADER_IDS; // first part is IDS
	else if (  IsNumeric(header1, true) && !strlen(header2) )
		return HEADER_LENGTH; // first part is number, and second is blank
	else if ( !strlen(header1) )
		return HEADER_BLANK; // header is blank
	else if ( ( IsHex(header1) || IsNumeric(header1) ) && strlen(header2) )
		return HEADER_RECORD; // header is a record
	else
		return HEADER_ERROR; // invalid header

}

bool IDSImp::IsNumeric(char * checkString, bool isUnsigned)
{ 
	char * strCheck = checkString;
    char c; 

    if ( !*strCheck) 
		return false; //trap empty strings 
    else if (*strCheck=='-' && isUnsigned == false) 
		strCheck++; 

    while ( *strCheck)
	{ 
        c=*strCheck++; 

        if (c < 48 || c > 57) 
			return false; 
    } 
     
    return true; 
}

bool IDSImp::IsHex(char * checkString)
{ 
	char * strCheck = checkString;
    char c; 

    if ( !*strCheck) 
		return false; //trap empty strings 
    else if (strCheck[0] =='0' && (strCheck[1] == 'x' || strCheck[1] == 'X') ) 
		strCheck+=2;

    while ( *strCheck)
	{ 
        c=*strCheck++; 
        if (!isxdigit(c)) 
			return false;
    } 
     
    return true; 
}
