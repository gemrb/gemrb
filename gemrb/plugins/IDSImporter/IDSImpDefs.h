#define MAX_LINES 400
#define MAX_VALUE_LENGTH 20
#define MAX_TEXT_LENGTH 60 // maximum text length in case IDS file doesn't specify
#define MAX_LINE_LENGTH MAX_VALUE_LENGTH + MAX_TEXT_LENGTH
#define MAX_HEADER_LENGTH 20

#define HEADER_IDS 1
#define HEADER_LENGTH 2
#define HEADER_BLANK 3
#define HEADER_RECORD 4
#define HEADER_ERROR -1

// need to transfer this to globals or something:
//#define SYMBOL_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays

