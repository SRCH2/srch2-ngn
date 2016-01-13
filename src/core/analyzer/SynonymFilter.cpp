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
/*
 * SynonymFilter.cpp
 *
 *  Created on: Jun 21, 2013
 */


#include "SynonymFilter.h"
#include "util/Logger.h"

#include <string>
#include <fstream>
#include "util/Assert.h"
#include "analyzer/AnalyzerContainers.h"

using namespace std;
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

SynonymFilter::SynonymFilter(TokenStream * tokenStream,
                             const SynonymContainer *_synonymContainer) :
    TokenFilter(tokenStream),
    synonymContainer(_synonymContainer)
{
	this->tokenStreamContainer = tokenStream->tokenStreamContainer; // copies the shared_ptr: sharedToken
	this->keepOriginFlag = synonymContainer->keepOrigin();
}

void SynonymFilter::clearState() {
    // clear the state of the filter in the upstream
    if (this->tokenStream != NULL)
        this->tokenStream->clearState();

    // clear our own states
    this->tokenBuffer.clear();
    this->emitBuffer.clear();
}

bool SynonymFilter::getSynonymValuesOf(const std::string &key, SynonymVector& synonyms) const
{
	return synonymContainer->getValue(key, synonyms);
}

bool SynonymFilter::isPrefixToken(const string& str) const {
	return synonymContainer->isPrefix(str);
}

void SynonymFilter::pushSynonymsOfExistingTokensInEmitBuffer() {
	bool flag;
	while (true) {
		if (this->tokenBuffer.size() == 0) {
			return;
		}
		flag = false;
		for (int i = this->tokenBuffer.size() - 1; i >= 0; i--) {
			std::string tempToken = "";
			for (int j = 0; j <= i; j++) {
				tempToken += this->tokenBuffer[j].term + " ";
			}
			tempToken = tempToken.substr(0, tempToken.length() - 1);
			SynonymVector synonyms;
			bool expand = getSynonymValuesOf(tempToken, synonyms);

			if (synonyms.size() > 0) {
				if (expand) { // checks the flag of Keeping origin word
					for (int k = 0; k <=i; k++) {
						this->emitBuffer.push_back(this->tokenBuffer[k]);
					}
				}
				unsigned beginPos = tokenBuffer[0].position;
				for (int j = 0 ; j < synonyms.size(); ++j) {
					AnalyzedTermInfo termInfo;
					termInfo.term = synonyms[j];
					termInfo.position = beginPos++;
					termInfo.charOffset = tokenBuffer[0].charOffset;
					termInfo.charLength = tokenBuffer.back().charOffset + tokenBuffer.back().charLength
							- tokenBuffer[0].charOffset;
					termInfo.analyzedTokenType = ANALYZED_SYNONYM_TOKEN;
					this->emitBuffer.push_back(termInfo);
				}
				for (int k = i; k >= 0; k--) {
					this->tokenBuffer.erase(this->tokenBuffer.begin() + k);
					flag = true;
				}
				break;
			}

		}
		if (!flag) {
			this->emitBuffer.push_back(this->tokenBuffer[0]);
			this->tokenBuffer.erase(this->tokenBuffer.begin());
		}
	}
	return;
}


void SynonymFilter::emitCurrentToken() {
	if (this->emitBuffer.size() > 0) {
		// setting the currentToken to the first element of the vector
		utf8StringToCharTypeVector(this->emitBuffer[0].term, tokenStreamContainer->currentToken);
		tokenStreamContainer->currentTokenPosition = this->emitBuffer[0].position;
		tokenStreamContainer->currentTokenOffset = this->emitBuffer[0].charOffset;
		tokenStreamContainer->type  = this->emitBuffer[0].analyzedTokenType;
		tokenStreamContainer->currentTokenLen  = this->emitBuffer[0].charLength;
		// removing the first element
		this->emitBuffer.erase(this->emitBuffer.begin() + 0);
	}
}

bool SynonymFilter::processToken() {
	while (true) {
		if (this->emitBuffer.size() > 0) {
			this->emitCurrentToken();
			return true;
		}
		// if increment returns false
		if (!this->tokenStream->processToken()) {
			/*
			* all the new incomming tokens come to the tokenBuffer.
			* all the tokens that have to wait to get emit, are in the temporaryBuffer
			* So, if the increment is false, and there is noting to emit, and the tokenBuffer
			* is empty, this function should return false.
			*/
			if (this->tokenBuffer.empty() && this->emitBuffer.empty()) {
				return false;
			} else {
				pushSynonymsOfExistingTokensInEmitBuffer();
				this->emitCurrentToken();
				return true;
			}
		} // end of increment=false

		AnalyzedTermInfo currTermInfo;
		// converts the charType to string
		charTypeVectorToUtf8String(tokenStreamContainer->currentToken, currTermInfo.term);
		currTermInfo.position = tokenStreamContainer->currentTokenPosition;
		currTermInfo.charOffset = tokenStreamContainer->currentTokenOffset;
		tokenStreamContainer->type = ANALYZED_ORIGINAL_TOKEN;
		currTermInfo.analyzedTokenType = tokenStreamContainer->type;
		currTermInfo.charLength = currTermInfo.term.size();

		// gives the number of prefixes found in the key set the map for the current token

		if (this->tokenBuffer.empty()) {
			bool isPrefixToken = this->isPrefixToken(currTermInfo.term);
			// if current token is a prefix token of any string which has synonyms. Then we want
			// to scan the largest synonym.
			// e.g for rule : new york => ny
			// token "new" does not have any synonyms but it is a prefix of "new york" which
			// has synonyms.
			// another example: new york => ny   and new york giants => Jints
			// Then we want to wait for possible "giants" token.
			if (isPrefixToken) {
				this->tokenBuffer.push_back(currTermInfo);
				continue;
			}
			SynonymVector synonyms;
			bool expand = this->getSynonymValuesOf(currTermInfo.term, synonyms);
			if (synonyms.size() == 0) {
				// not a prefix token and not a synonym then just send it to next filter.
				this->emitBuffer.push_back(currTermInfo);
				this->emitCurrentToken();
				return true;
			} else {
				if (expand) {
					this->emitBuffer.push_back(currTermInfo);
				}
				// now add its synonyms
				for (unsigned i = 0; i < synonyms.size(); ++i) {
					AnalyzedTermInfo termInfo;
					termInfo.term = synonyms[i];
					termInfo.position = tokenStreamContainer->currentTokenPosition;
					termInfo.charOffset = tokenStreamContainer->currentTokenOffset;
					termInfo.charLength = currTermInfo.charLength;
					termInfo.analyzedTokenType = ANALYZED_SYNONYM_TOKEN;
					this->emitBuffer.push_back(termInfo);
				}
				this->emitCurrentToken();
				return true;
			}
		// if the buffer is not empty
		} else {
			// this will append all previous tokens in the tokenBuffer
			string previousTokens = "";
			for (int i = 0; i < this->tokenBuffer.size(); i++) {
				previousTokens = previousTokens + this->tokenBuffer[i].term + " ";
			}
			bool isPrefixToken = this->isPrefixToken(previousTokens + currTermInfo.term);
			if (isPrefixToken) {
				this->tokenBuffer.push_back(currTermInfo);
				continue;
			}
			SynonymVector synonyms;
			// previousCurrentToken is previous tokens followed by current token
			bool expand = this->getSynonymValuesOf(previousTokens + currTermInfo.term, synonyms);
			// if there is NOT any match for the elements in the buffer that followed by current token
			if (synonyms.size() == 0) {
				pushSynonymsOfExistingTokensInEmitBuffer();
				tokenBuffer.push_back(currTermInfo);
				this->emitCurrentToken();
				return true;
			} else if (synonyms.size() > 0) {
				/*
				* Now we should check if they have a complete match OR
				* again this combination of buffer and current token is a prefix of a key in the synonym or not
				* if it is complete we should just replace them with the synonym
				* if it is still not complete, we have to push the current token into the buffer and move on.
				*/
				if (expand) {
					for (int i =0; i < tokenBuffer.size(); i++) {
						this->emitBuffer.push_back(tokenBuffer[i]);
					}
					this->emitBuffer.push_back(currTermInfo);
				}
				// push its synonnyms
				unsigned beginPos = tokenBuffer[0].position;
				for (int i = 0; i < synonyms.size(); i++) {
					AnalyzedTermInfo termInfo;
					termInfo.term = synonyms[i];
					termInfo.position = beginPos++;
					termInfo.charOffset = tokenBuffer[0].charOffset;
					termInfo.charLength = currTermInfo.charOffset + currTermInfo.charLength
							- tokenBuffer[0].charOffset;
					termInfo.analyzedTokenType = ANALYZED_SYNONYM_TOKEN;
					this->emitBuffer.push_back(termInfo);
				}
				this->emitCurrentToken();
				this->tokenBuffer.clear();
				return true;
			}
		}
	}
	return false; // The function will not reach here and this return is for avoiding the warnings.
}

SynonymFilter::~SynonymFilter() {
}

}
}
