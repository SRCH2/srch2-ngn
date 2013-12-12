// $Id: StopFilter.cpp 3074 2013-21-06 22:26:36Z iman $

/*
 * SynonymFilter.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: iman
 */

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
		const std::string &synonymFilterFilePath,
		const SynonymKeepOriginFlag &synonymKeepOriginFlag) :
		TokenFilter(tokenStream), synonymContainer(SynonymContainer::getInstance())  {
	this->tokenStreamContainer = tokenStream->tokenStreamContainer; // copies the shared_ptr: sharedToken
	this->keepOriginFlag = synonymKeepOriginFlag;
}


pair<SynonymTokenType, std::string> SynonymFilter::getValuePairOf(const std::string &key) {
	pair<SynonymTokenType, std::string> valuePair;
	synonymContainer.getValue(key, valuePair);
	return valuePair;
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
				tempToken += this->tokenBuffer[j] + " ";
			}
			tempToken = tempToken.substr(0, tempToken.length() - 1);
			pair<SynonymTokenType, std::string> valuePair;
			synonymContainer.getValue(tempToken, valuePair);

			if (valuePair.first == SYNONYM_COMPLETE_ONLY || valuePair.first == SYNONYM_PREFIX_AND_COMPLETE) {
				if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
					for (int k = 0; k <=i; k++) {
						this->emitBuffer.push_back(this->tokenBuffer[k]);
					}
				}
				this->emitBuffer.push_back(valuePair.second);
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
	// setting the currentToken to the first element of the vector
	utf8StringToCharTypeVector(this->emitBuffer[0], tokenStreamContainer->currentToken);
	// removing the first element
	this->emitBuffer.erase(this->emitBuffer.begin() + 0);
}

void SynonymFilter::clearState() {
    // clear the state of the filter in the upstream
	if (this->tokenStream != NULL)
		this->tokenStream->clearState();

	// clear our own states: nothing to do.
}


bool SynonymFilter::processToken() {
	while (true) {
		// if increment returns false
		if (!this->tokenStream->processToken()) {
			/*
			* all the new incomming tokens come to the tokenBuffer.
			* all the tokens that have to wait to get emit, are in the temporaryBuffer
			* So, if the increment is false, and there is noting to emit, and the tokenBuffer is empty, this function should return false.
			*/
			if (this->tokenBuffer.empty() && this->emitBuffer.empty()) {
				return false;
			} else {
				pushSynonymsOfExistingTokensInEmitBuffer();
				this->emitCurrentToken();
				return true;
			}
		} // end of increment=false
		std::string currentToken = "";
		// converts the charType to string
		charTypeVectorToUtf8String(tokenStreamContainer->currentToken, currentToken);
		// gives the number of prefixes found in the key set the map for the current token

		// if increment returns true
		if (this->tokenBuffer.empty()) {
			pair<SynonymTokenType, std::string> currentPair = this->getValuePairOf(currentToken);
			// if the currentToken is not prefix of any of the keys in the map
			// if it is zero we have to emit this token (because there is not any synonym match for it)
			if (currentPair.first == SYNONYM_NOT_PREFIX_NOT_COMPLETE) {
				this->emitBuffer.push_back(currentToken);
				this->emitCurrentToken();
				return true;
			// if there is one synonym match for the new token
			} else if (currentPair.first == SYNONYM_COMPLETE_ONLY) {
				if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
					this->emitBuffer.push_back(currentToken); // this is for adding the original tokens.
				}
				this->emitBuffer.push_back(currentPair.second);
				this->emitCurrentToken();
				return true;
			// if there is more than one, push it to the buffer.
			} else {
				this->tokenBuffer.push_back(currentToken);
			}
		// if the buffer is not empty
		} else {
			// this will append all previous tokens in the tokenBuffer
			string previousTokens = "";
			for (int i = 0; i < this->tokenBuffer.size(); i++) {
				previousTokens = previousTokens + this->tokenBuffer[i] + " ";
			}
			// previousCurrentToken is previous tokens followed by current token
			pair<SynonymTokenType, std::string> previousCurrentPair = this->getValuePairOf(previousTokens + currentToken);
			// if there is NOT any match for the elements in the buffer that followed by current token
			if ( previousCurrentPair.first == SYNONYM_NOT_PREFIX_NOT_COMPLETE) {
				pair<SynonymTokenType, std::string> currentPair = this->getValuePairOf(currentToken);
				pushSynonymsOfExistingTokensInEmitBuffer();
				if (currentPair.first == SYNONYM_NOT_PREFIX_NOT_COMPLETE) {
					this->emitBuffer.push_back(currentToken);
				} else if (currentPair.first == SYNONYM_COMPLETE_ONLY) {
					if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
						this->emitBuffer.push_back(currentToken); // this is for adding the original tokens.
					}
					this->emitBuffer.push_back(currentPair.second);
				} else {
					this->tokenBuffer.push_back(currentToken);
				}
				this->emitCurrentToken();
				return true;
				// if there is ONE match for the elements in the buffer that followed by current token
			} else if (previousCurrentPair.first == SYNONYM_COMPLETE_ONLY) {
				/*
				* Now we should check if they have a complete match OR
				* again this combination of buffer and current token is a prefix of a key in the synonym or not
				* if it is complete we should just replace them with the synonym
				* if it is still not complete, we have to push the current token into the buffer and move on.
				*/
				if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
					for (int i =0; i < tokenBuffer.size(); i++) {
						this->emitBuffer.push_back(tokenBuffer[i]);
					}
					this->emitBuffer.push_back(currentToken);
				}
				this->emitBuffer.push_back(previousCurrentPair.second);
				this->emitCurrentToken();
				this->tokenBuffer.clear();
				return true;
			} else {
				this->tokenBuffer.push_back(currentToken);
			}
		}
	}
	return false; // The function will not reach here and this return is for avoiding the warnings.
}

SynonymFilter::~SynonymFilter() {
}

}
}
