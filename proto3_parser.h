#ifdef PARSER_API
#else
#define PARSER_API extern "C" _declspec(dllimport)
#endif

PARSER_API char* parser(char* byte_str, int length, const char* packet_id);

PARSER_API void free_memory(char* memory);