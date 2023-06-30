#ifdef PARSER_API
#else
#define PARSER_API extern "C" _declspec(dllimport)
#endif

#include <json/json.h>

PARSER_API const char* parser(const char* byte_str, int length, const char* packet_id);