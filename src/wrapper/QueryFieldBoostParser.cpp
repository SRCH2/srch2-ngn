/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
