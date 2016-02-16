#include "util/ParserUtility.h"
#include "RegexConstants.h"
#include "QueryFieldBoostParser.h"

using srch2::util::Logger;

namespace srch2 {
namespace httpwrapper {

bool parseQfAttribute(std::string& input, std::string& output) {
  boost::regex re(QF_ATTRIBUTE_REGEX_STRING); //TODO: make compile time
  if(doParse(input, re, output)) {
    ASSERT((*(--output.end()) == '^')); 
    output.erase(--output.end()); // erases the '^' character at end of string
    //erase the begin + sign if a conjuctive
    if(*(output.begin()) == '+') {
      output.erase(output.begin());
    }
    return true;
  }
  return false;
}

bool parseQfBoost(std::string& input, std::string& output) {
  boost::regex re(NUMBERS_REGEX_STRING); //TODO: make compile time
  
  return doParse(input, re, output);
}
  

bool srch2::httpwrapper::QueryFieldBoostParser::parseAndAddCriterion(
  QueryFieldBoostContainer& qfcontainer, std::string& parameterString) {
  string currentParameterString= parameterString;
  do {
    std::string qfField;
    std::string qfBoost;
    struct QueryFieldAttributeBoost attributeBoost;
    // no more Parameters to parse
    if(!parseQfAttribute(currentParameterString, qfField)) {
     Logger::error("Error while parsing query field boost:"
          "expected ^ after attribute name in %s", 
          currentParameterString.c_str());
      return false;
    }
    attributeBoost.attribute= qfField;
    if(!parseQfBoost(currentParameterString, qfBoost)) {
      Logger::error("Error while parsing query field boost:"
          "boost integer value expected after %s attribute", qfField.c_str());
      return false;
    }
    attributeBoost.boost= static_cast<float>(strtod(qfBoost.c_str(),NULL));
    ASSERT(attributeBoost.boost);
    qfcontainer.boosts.push_back(attributeBoost);
  } while(*currentParameterString.begin() == '+');

  return currentParameterString.empty();
}

}}
