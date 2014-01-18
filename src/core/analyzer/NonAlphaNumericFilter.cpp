/*
 * NonAlphaNumericFilter.cpp
 *
 *  Created on: Nov 13, 2013
 *      Author: sbisht
 */

#include "NonAlphaNumericFilter.h"

namespace srch2 {
namespace instantsearch {

NonAlphaNumericFilter::NonAlphaNumericFilter(TokenStream *tokenStream,
                                             const ProtectedWordsContainer *_protectedWordsContainer) :
    TokenFilter(tokenStream),
    protectedWordsContainer(_protectedWordsContainer)
{
    this->tokenStreamContainer = tokenStream->tokenStreamContainer;
}

void NonAlphaNumericFilter::clearState() {
    // clear the state of the filter in the upstream
    if (this->tokenStream != NULL)
        this->tokenStream->clearState();
  
    // clear our own states
    while (!this->internalTokenBuffer.empty())
        this->internalTokenBuffer.pop();
}

/*
 *    This filter receives tokens from upstream filter/tokenizer and then further tokenizes
 *    them based on delimiters. It is advised to have this filter right after the tokenizer.
 *
 *    1. If the token is a protected token, then we do not analyze it further. Instead, we just
 *       send it downstream
 *    2. If the token is not a protected token then we try to tokenize it based on the delimiters
 *       and store the tokens in an internal buffer. This buffer should be flushed to downstream
 *       filters first before asking for new tokens from the upstream tokenizer.
 *
 *    input tokens = { I love c++ and java-script programming }
 *    output tokens = {  I love c++ and java-script programming }  , c++ is a protected keyword
 *    but java-script is not a protected keyword.
 */
bool NonAlphaNumericFilter::processToken()
{
	while(1) {
		// if we have tokens in the filter's internal buffer then flush them out one by one before
		// requesting a new token from the upstream.
		if (internalTokenBuffer.size() == 0) {
			if (this->tokenStream != NULL && !this->tokenStream->processToken()) {
				return false;
			}
			string currentToken;
			charTypeVectorToUtf8String(this->tokenStreamContainer->currentToken, currentToken);

			if (isProtectWord(currentToken)) {
				return true;      // Do not apply any filter on protected keywords such as C++
			}

			unsigned currOffset = 0;
			const vector<CharType> & charTypeBuffer = this->tokenStreamContainer->currentToken;
			vector<CharType> tempToken;
			// Try to tokenize keywords based on delimiters
			while (currOffset < charTypeBuffer.size()) {
				const CharType& c = charTypeBuffer[currOffset];
				currOffset++;
				switch (characterSet.getCharacterType(c)) {
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
			if (!tempToken.empty()) {  // whatever is left over, push it to the internal buffer.
				internalTokenBuffer.push(tempToken);
				tempToken.clear();
			}
			if (internalTokenBuffer.size() > 0) {
				// put first element from the internal token buffer to a shared token buffer for other
				// filters to consume.  e.g internal buffer = {java script}, put "java" to
				// the shared token.
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
