
#ifndef __REGEX_CONSTANTS__
#define __REGEX_CONSTANTS__

#include <string>

using namespace std;

namespace srch2 {
namespace httpwrapper {
// main query regex strings


// local parameter regex strings
const string LP_KEY_REGEX_STRING = "^\\s*[\\w_]+";
const string LP_KEY_VAL_DELIMETER_REGEX_STRING = "^\\s*=\\s*";
const string LP_VALUE_REGEX_STRING = "^[\\w_]+(,[\\w_]+)*";
// keyword regex strings
const string BOOST_REGEX_STRING = "\\^\\d+";
const string CHECK_FUZZY_NUMBER_REGEX_STRING = "^\\0?\\.\\d+";
const string NUMBERS_REGEX_STRING = "\\d+";
const string FIELD_AND_BOOL_OP_DELIMETER_REGEX_STRING = "\\.";
const string FIELD_OR_BOOL_OP_DELIMETER_REGEX_STRING = "\\+";
const string TERM_BOOL_OP_REGEX_STRING = "^(AND|OR)";
const string MAIN_QUERY_TERM_FIELD_REGEX_STRING = "^([\\w_]+((\\.|\\+)[\\w_]*)*|\\*)\\s*:";
const string MAIN_QUERY_KEYWORD_REGEX_STRING = "^[^\\*\\^\\~\\s:]+";
const string MAIN_QUERY_ASTERIC_KEYWORD_REGEX_STRING = "^\\*\\s*";
const string PREFIX_MODIFIER_REGEX_STRING = "^\\*";
const string BOOST_MODIFIER_REGEX_STRING = "^\\^\\d*";
const string FUZZY_MODIFIER_REGEX_STRING = "^~((0?\\.(\\d)+)|1(\\.0+)?)?";
const string PHRASE_BOOST_MODIFIER_REGEX_STRING = "^\\^\\d+";
const string PROXIMITY_MODIFIER_REGEX_STRING = "^~\\d+";
const string FQ_FIELD_REGEX_STRING = "^-{0,1}[\\w_]+\\s*:";
const string COMPLEX_TERM_REGEX_STRING = "^boolexp\\$";

// filter query regex strings


const string FQ_TERM_BOOL_OP_REGEX_STRING = "^(AND|OR)";
const string FQ_RANGE_QUERY_KEYWORD_REGEX_STRING = "[^:]+\\]";
const string FQ_ASSIGNMENT_KEYWORD_REGEX_STRING = "^[^\\s:]+"; // no space allowed
const string FQ_COMPLEX_EXPRESSION_REGEX_STRING = "[^\\$]+";
const string FQ_FIELD_KEYWORD_DELIMETER_REGEX_STRING = "\\s+TO\\s+";
}

/* Query field boost regex strings, used to match string in the qf= section:
   matches from the begining of string up to and including the first ^.
     Use cases are as follows: qf=title^100+record^100
       - first regex call gets string title^100+record^100
         and this expression matches title^
       - second regex call get string record^100 and this expression
         matches record^ */
const string QF_ATTRIBUTE_REGEX_STRING = "^[^\\^]*\\^";
}

#endif // __REGEX_CONSTANTS__
