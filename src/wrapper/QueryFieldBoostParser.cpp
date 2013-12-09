// $Id$ 12/01/13 RJ

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright 2013 SRCH2 Inc. All rights reserved
 */

#include "ParserUtility.h"
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
    if((*(output.begin()) == '+')) {
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
    attributeBoost.boost= std::atoi(qfBoost.c_str());
    ASSERT(!attributeBoost.boost);
    qfcontainer.boosts.push_back(attributeBoost);
  } while(*currentParameterString.begin() == '+');

  return currentParameterString.empty();
}

}}
