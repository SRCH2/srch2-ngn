
#ifndef __REGEX_CONSTANTS__
#define __REGEX_CONSTANTS__

#include <string>

using namespace std;

namespace srch2 {
namespace httpwrapper {
// main query regex strings


// local parameter regex strings
const string LP_KEY_REGEX_STRING = "\\s*[\\w_]+";
const string LP_KEY_VAL_DELIMETER_REGEX_STRING = "\\s*=\\s*";
const string LP_VALUE_REGEX_STRING = "[\\w_]+(,[\\w_]+)*";
// keyword regex strings
const string BOOST_REGEX_STRING = "\\^\\d+";
const string CHECK_FUZZY_NUMBER_REGEX_STRING = "\\~\\.\\d+";
const string NUMBERS_REGEX_STRING = "\\d+";
const string FIELD_AND_BOOL_OP_DELIMETER_REGEX_STRING = "\\.";
const string FIELD_OR_BOOL_OP_DELIMETER_REGEX_STRING = "\\+";
const string TERM_BOOL_OP_REGEX_STRING = "(AND|OR)";
const string MAIN_QUERY_TERM_FIELD_REGEX_STRING = "([\\w_]+((\\.|\\+)[\\w_]*)*|\\*)\\s*:";
const string MAIN_QUERY_KEYWORD_REGEX_STRING = "[^\\*\\^\\~\\s]+";
const string MAIN_QUERY_ASTERIC_KEYWORD_REGEX_STRING = "\\*\\s*";
const string PREFIX_MODIFIER_REGEX_STRING = "\\*";
const string BOOST_MODIFIER_REGEX_STRING = "\\^\\d*";
const string FUZZE_MODIFIER_REGEX_STRING = "~(\\.\\d){0,1}";
const string FQ_FIELD_REGEX_STRING = "-{0,1}[\\w_]+\\s*:";
const string COMPLEX_TERM_REGEX_STRING = "CMPLX\\$";


// filter query regex strings


const string FQ_TERM_BOOL_OP_REGEX_STRING = "(AND|OR)";
const string FQ_RANGE_QUERY_KEYWORD_REGEX_STRING = ".+\\]";
const string FQ_ASSIGNMENT_KEYWORD_REGEX_STRING = "[^\\s]+"; // no space allowed
const string FQ_COMPLEX_EXPRESSION_REGEX_STRING = "[^\\$]+";

}
}

#endif // __REGEX_CONSTANTS__
