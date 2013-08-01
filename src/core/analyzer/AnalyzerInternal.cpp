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
using std::pair;
using namespace std;

namespace srch2 {
namespace instantsearch {

/**
 * Helper function to tokenizeRecord
 * @param attribbute
 * @param position
 * @return an unsigned with bits set with the following logic:
 * Attribute -> First 8bits -> Attribute in which the token hit occurred
 * Position -> Last 24 bits -> Position within the attribute where the token hit occurred.
 */
unsigned setAttributePositionBitVector(unsigned attribute, unsigned position) {
	///assert that attribute is less than maximum allowed attributes
	ASSERT(attribute < 0xff);

	///assert that position is less than maximum allowed document size
	ASSERT(position < 0xffffff);

	return ((attribute + 1) << 24) + (position & 0xffffff);

//    return 1 << attribute;
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
	tokenStreamContainer.reset(new TokenStreamContainer);
	this->stemmerType = analyzerInternal.stemmerType;
	this->stemmerFilePath = analyzerInternal.stemmerFilePath;
	this->stopWordFilePath = analyzerInternal.stopWordFilePath;
	this->synonymFilePath = analyzerInternal.synonymFilePath;
	this->synonymKeepOriginFlag = analyzerInternal.synonymKeepOriginFlag;
}

AnalyzerInternal::AnalyzerInternal(const StemmerNormalizerFlagType &stemmerFlag,
		const string &recordAllowedSpecialCharacters,
		const string &stemmerFilePath,
		const string &stopWordFilePath,
		const string &synonymFilePath,
		const SynonymKeepOriginFlag &synonymKeepOriginFlag) {

	this->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
	CharSet::setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
	prepareRegexExpression();
	tokenStreamContainer.reset(new TokenStreamContainer);
	this->stemmerType = stemmerFlag;
	this->stemmerFilePath = stemmerFilePath;
	this->stopWordFilePath = stopWordFilePath;
	this->synonymFilePath = synonymFilePath;
	this->synonymKeepOriginFlag = synonymKeepOriginFlag;

}

void AnalyzerInternal::loadData(const string &s) const {
	std::vector<CharType> charVector;
	utf8StringToCharTypeVector(s, charVector); //clean the string and convert the string to CharTypeVector
	this->tokenStreamContainer->currentToken.clear();
	this->tokenStreamContainer->completeCharVector = charVector;
	this->tokenStreamContainer->offset = 0;
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
	vector <string> tokens;
	vector <string>::iterator tokenIterator;
	unsigned positionIterator;
	unsigned size;
	for (unsigned attributeIterator = 0;
			attributeIterator < schema->getNumberOfSearchableAttributes();
			attributeIterator++) {
		string *attributeValue = record->getSearchableAttributeValue(
				attributeIterator);
		if (attributeValue != NULL) {
			tokens.clear();
			loadData(*attributeValue);
			string currentToken = "";
			while (tokenStream->processToken()) //process the token one by one
			{
				vector<CharType> charVector;
				charVector = tokenStream->getProcessedToken();
				charTypeVectorToUtf8String(charVector, currentToken);
				tokens.push_back(currentToken);
				//cout<<currentToken<<endl;
			}
			size = tokens.size();

			positionIterator = 1;
			for (tokenIterator = tokens.begin(); tokenIterator != tokens.end();
					++tokenIterator, ++positionIterator) {

				///TODO OPT: Remove this comparison and make sure, split returns no empty strings.
				if (tokenIterator->size()) {
					//Convert to lowercase
					std::transform(tokenIterator->begin(), tokenIterator->end(),
							tokenIterator->begin(), ::tolower);

					tokenAttributeHitsMap[*tokenIterator].attributeList.push_back(
							setAttributePositionBitVector(attributeIterator,
									positionIterator));
				}
			}

		}
	}
}

}
}
