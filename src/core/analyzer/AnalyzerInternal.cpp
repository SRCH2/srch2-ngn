// $Id: AnalyzerInternal.cpp 3375 2013-05-24 06:54:28Z iman $

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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include <instantsearch/Record.h>
#include <instantsearch/Schema.h>
#include <util/Assert.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "AnalyzerInternal.h"
#include "util/encoding.h"
#include "AnalyzerContainers.h"

using std::pair;
using namespace std;

namespace srch2 {
namespace instantsearch {


bool isEmpty(const string &inString) {
    return inString.compare("") == 0;
}

/*
 * Important:
 *   Since this AnalyzerInternal class can be inherited by other superclasses, it's a common practice to initialize these
 *   parameters in this superclass, rather than doing the initialization in the subclasses. In fact, if we let the subclasses
 *   do the initialization, for some reason our engine doesn't work on Android.
 */

AnalyzerInternal::AnalyzerInternal(const AnalyzerInternal &analyzerInternal) {
    this->recordAllowedSpecialCharacters = analyzerInternal.recordAllowedSpecialCharacters;
    prepareRegexExpression();

    this->stemmer = analyzerInternal.stemmer;
    this->stopWords = analyzerInternal.stopWords;
    this->protectedWords = analyzerInternal.protectedWords;
    this->synonyms = analyzerInternal.synonyms;
}

AnalyzerInternal::AnalyzerInternal(const StemmerContainer *stemmer,
                                   const StopWordContainer *stopWords,
                                   const ProtectedWordsContainer *protectedWords,
                                   const SynonymContainer *synonyms,
                                   const std::string &allowedSpecialCharacters
                                   )
{
    this->recordAllowedSpecialCharacters = allowedSpecialCharacters;
    this->stemmer = stemmer;
    this->stopWords = stopWords;
    this->protectedWords = protectedWords;
    this->synonyms = synonyms;
}

string AnalyzerInternal::applyFilters(string input, bool isPrefix) {

    this->tokenStream->fillInCharacters(input, isPrefix);

    string result = "";
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        string tmp;
        charTypeVectorToUtf8String(charVector, tmp);
        result += tmp;
        break; //only returns the first output of filters
    }
    return result;
}


void AnalyzerInternal::clearFilterStates() {
  // clear the state of each filter on the chain
  if (tokenStream != NULL) {
    tokenStream->clearState();
  }
}

void AnalyzerInternal::load(boost::archive::binary_iarchive &ia)
{
    ia >> *this;

    Logger::debug("#### AnalyzerInternal Variables:   \n");
    if (stemmer != NULL) {
        Logger::debug("Stemmer File Path :            %s\n",
                      stemmer->getFilePath().c_str());
    }
    if (stopWords != NULL) {
        Logger::debug("Stop Word File Path:           %s\n",
                      stopWords->getFilePath().c_str());
    }
    if (synonyms != NULL) {
        Logger::debug("Synonym File Path is:          %s\n",
                      synonyms->getFilePath().c_str());
    }
    Logger::debug("Synonym Keep Origin Flag is:   %d\n", this->synonyms->keepOrigin());
    Logger::debug("Analyzer Type:                 %d\n\n\n", this->analyzerType);
}

/**
 * Function to tokenize a given record.
 * @param[in] record
 * @param[in, out] tokenAttributeHitsMap
 *
 * Pseudocode:
 * 1. map<string, TokenAttributeHits > tokenAttributeHitsMap of a record is a map from unique tokens to their corresponding attributeHits information(Refer Struct TokenAttributeHits )
 * 2. First, we iterate over the record attributes. So, for each recordAttribute
 *      1. Boost::split is used to get the tokens from attributeValue.
 *      2. We iterate over these tokens and fill the map
 */
void AnalyzerInternal::tokenizeRecord(const Record *record,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) const {
    tokenAttributeHitsMap.clear();
    const Schema *schema = record->getSchema();
    // token string to vector<CharType>
    vector<AnalyzedTermInfo> tokens;
    for (unsigned attributeIterator = 0;
            attributeIterator < schema->getNumberOfSearchableAttributes();
            attributeIterator++) {
    	/*
    	 * Multi Value attributes
    	 * Position information example:
    	 * Example : Suppose the value of this attribute is <'A B C', 'D E F', 'G H'>
    	 * Assuming the bump value is 100000, after this iteration, the positions given to these tokens are
    	 * Tokens : <A,1> <B,2> <C,3> <D,100004> <E,100005> <F, 100006> <G, 200007 > <H, 200008>
    	 *
    	 * For char offset we do not use offset bumps. Instead we treat the multi-value attributes
    	 * as one attribute concatenated via special delimiter.
    	 * Example: ["apple", "berry"]  --> "apple $$ berry"
    	 * offset : apple : 1, berry : 10
    	 */
        vector<string> attributeValues;
        record->getSearchableAttributeValues(attributeIterator , attributeValues);

        if (!attributeValues.empty()) {
			tokens.clear();
			unsigned prevAttrCombinedLen = 0;
        	for(unsigned valueOffset = 0 ; valueOffset != attributeValues.size() ; ++valueOffset){
        		string attributeValue = attributeValues.at(valueOffset);
				this->tokenStream->fillInCharacters(attributeValue);
				string currentToken = "";
				while (tokenStream->processToken()) //process the token one by one
				{
					vector<CharType> charVector;
					charVector = tokenStream->getProcessedToken();
					unsigned termPosition = tokenStream->getProcessedTokenPosition();
					unsigned charOffset = tokenStream->getProcessedTokenCharOffset();
					AnalyzedTokenType tokenType =  tokenStream->getProcessedTokentype();
					unsigned charLen = tokenStream->getProcessedTokenLen();
					charTypeVectorToUtf8String(charVector, currentToken);
					if (tokenType == ANALYZED_SYNONYM_TOKEN) {
						vector<string> currentTokens;
						unsigned synonymCharOffset = prevAttrCombinedLen + charOffset;
						boost::algorithm::split(currentTokens, currentToken, boost::is_any_of(" "));
						for (unsigned i = 0; i < currentTokens.size(); ++i) {
							if (i != 0) {
								// For multi word synonym we do not store char offset for non-first token.
								// e.g. nyc => new york , and "new york" begin the generated synonym,
								// "new" offset is same as "nyc" and "york" offset is 0.
								synonymCharOffset = 0;
							}
							AnalyzedTermInfo pterm = {currentTokens[i], termPosition + i + valueOffset * MULTI_VALUED_ATTRIBUTE_POSITION_BUMP,
									synonymCharOffset , charLen, tokenType};
							tokens.push_back(pterm);
						}
					} else {
						// Bumps are added to the positions after tokenizer gives us the values.
						AnalyzedTermInfo pterm = {currentToken, termPosition + valueOffset * MULTI_VALUED_ATTRIBUTE_POSITION_BUMP,
								prevAttrCombinedLen+ charOffset, charLen, tokenType};
						tokens.push_back(pterm);
					}
				}
				prevAttrCombinedLen += attributeValue.length() + MULTI_VAL_ATTR_DELIMITER_LEN /*multivalue separator " $$ "*/;
        	}
			for (unsigned i = 0; i< tokens.size(); ++i) {
				if (tokens[i].term.size()) {
					tokenAttributeHitsMap[tokens[i].term].attributeIdList.push_back(attributeIterator);
					tokenAttributeHitsMap[tokens[i].term].positionOfTermInAttribute.push_back(tokens[i].position);
					tokenAttributeHitsMap[tokens[i].term].charOffsetOfTermInAttribute.push_back(
							tokens[i].charOffset);
					tokenAttributeHitsMap[tokens[i].term].charLensOfTermInAttribute.push_back(
							tokens[i].charLength);
					tokenAttributeHitsMap[tokens[i].term].typesOfTermInAttribute.push_back(
							tokens[i].analyzedTokenType);
				}
			}

        }
    }
}

//token utf-8 string to vector<vector<CharType> >
void AnalyzerInternal::tokenizeQuery(const string &queryString,
        vector<AnalyzedTermInfo> &queryKeywords, bool isPrefix) const {
    queryKeywords.clear();
    this->tokenStream->fillInCharacters(queryString);
    string currentToken = "";
    while (tokenStream->processToken()) //process the token one by one
    {
        vector<CharType> charVector;
        charVector = this->tokenStream->getProcessedToken();
        unsigned position = this->tokenStream->getProcessedTokenPosition();
        charTypeVectorToUtf8String(charVector, currentToken);
        AnalyzedTermInfo pterm = {currentToken, position};
        queryKeywords.push_back(pterm);
        //cout<<currentToken<<endl;
    }

    if (queryKeywords.size() == 1 && isEmpty(queryKeywords[0].term))
        queryKeywords.clear();
}

bool queryIsEmpty(string str) {
    return str.empty();
}


const string& AnalyzerInternal::getRecordAllowedSpecialCharacters() const{
    return this->recordAllowedSpecialCharacters;
}

TokenStream* AnalyzerInternal::getTokenStream() {
	return this->tokenStream;
}
}
}
