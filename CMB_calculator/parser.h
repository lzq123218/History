#ifndef CAL_PARSE
#define CAL_PARSE

#include <string>
#include <map>

typedef std::pair<CStringW, CStringW> value_pair;
typedef std::map< CStringW, CStringW > propertys;

enum PARSER_STATE {

    PARSE_END = 0,
    PARSE_PRDLIST,
    PARSE_RATIO
};


int GetAndParse( const wchar_t* ProductCode, propertys& out );

#endif