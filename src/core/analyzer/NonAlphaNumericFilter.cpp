/*
 * NonAlphaNumericFilter.cpp
 *
 *  Created on: Nov 13, 2013
 *      Author: sbisht
 */

#include "NonAlphaNumericFilter.h"

namespace srch2 {
namespace instantsearch {

NonAlphaNumericFilter::NonAlphaNumericFilter(TokenStream *tokenStream):
		TokenFilter(tokenStream), protectedWordsContainer(ProtectedWordsContainer::getInstance()) {
	this->tokenStreamContainer = tokenStream->tokenStreamContainer;
}

bool NonAlphaNumericFilter::processToken() {
	while(1) {
		if (internalTokenBuffer.size() == 0) {
			if (!this->tokenStream->processToken()) {
				return false;
			}
			string currentToken;
			charTypeVectorToUtf8String(this->tokenStreamContainer->currentToken, currentToken);

			if (isProtectWord(currentToken)) {
				return true;  // Do not apply any filter on protected keywords
			}

			unsigned currOffset = 0;
			const vector<CharType> & charTypeBuffer = this->tokenStreamContainer->currentToken;
			vector<CharType> tempToken;
			while (currOffset < charTypeBuffer.size()) {
				const CharType& c = charTypeBuffer[currOffset];
				currOffset++;
				switch (CharSet::getCharacterType(c)) {
				case CharSet::DELIMITER_TYPE:
				case CharSet::WHITESPACE:
					if (!tempToken.empty()) {
						internalTokenBuffer.push(tempToken);
						tempToken.clear();
					}
					break;
				default:
					tempToken.push_back(c);
					break;
				}
			}
			if (!tempToken.empty()) {
				internalTokenBuffer.push(tempToken);
				tempToken.clear();
			}
			if (internalTokenBuffer.size() > 0) {
				this->tokenStreamContainer->currentToken = internalTokenBuffer.front();
				internalTokenBuffer.pop();
				return true;
			}
		} else {
			this->tokenStreamContainer->currentToken = internalTokenBuffer.front();
			this->tokenStreamContainer->currentTokenPosition++;
			internalTokenBuffer.pop();
			return true;
		}
	}
	return false; // to avoid compiler warning
}


NonAlphaNumericFilter::~NonAlphaNumericFilter() {
}

} /* namespace instanstsearch */
} /* namespace srch2 */
