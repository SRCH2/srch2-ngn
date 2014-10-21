//$Id: URLParser.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include "URLParser.h"

namespace srch2
{
namespace httpwrapper
{


//for search
const char URLParser::queryDelimiter = '+';
const char URLParser::filterDelimiter = ':';
const char URLParser::fieldsAndDelimiter = ',';
const char URLParser::fieldsOrDelimiter = '.';

const char* const URLParser::searchTypeParamName = "type";
const char* const URLParser::keywordsParamName = "q";
const char* const URLParser::termTypesParamName = "termtypes";
const char* const URLParser::termBoostsParamName = "termboosts";
const char* const URLParser::fuzzyQueryParamName = "fuzzy";
const char* const URLParser::similarityBoostsParamName = "simboost";
const char* const URLParser::resultsToRetrieveStartParamName = "start";
const char* const URLParser::resultsToRetrieveLimitParamName = "limit";
const char* const URLParser::attributeToSortParamName = "sortby";
const char* const URLParser::orderParamName = "order";
const char* const URLParser::lengthBoostParamName = "lengthboost";
const char* const URLParser::jsonpCallBackName = "callback";

const char* const URLParser::leftBottomLatitudeParamName = "lb_lat";
const char* const URLParser::leftBottomLongitudeParamName = "lb_lng";
const char* const URLParser::rightTopLatitudeParamName = "rt_lat";
const char* const URLParser::rightTopLongitudeParamName = "rt_lng";

const char* const URLParser::centerLatitudeParamName = "ct_lat";
const char* const URLParser::centerLongitudeParamName = "ct_lng";
const char* const URLParser::radiusParamName = "radius";
const char* const URLParser::nameParamName = "exported_data_file";
const char* const URLParser::logNameParamName = "log_data_file";
const char* const URLParser::setParamName = "set";

// Schema will be used in Attribute-based search to set attribute bitmap.

}}
