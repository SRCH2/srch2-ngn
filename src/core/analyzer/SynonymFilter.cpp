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

#include <string>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include "util/Assert.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

SynonymFilter::SynonymFilter(TokenOperator * tokenOperator,
		const std::string &synonymFilterFilePath,
		const SynonymKeepOriginFlag &synonymKeepOriginFlag) :
		TokenFilter(tokenOperator), synonymDelimiter("=>") {
	this->sharedToken = tokenOperator->sharedToken; // copies the shared_ptr: sharedToken
	this->createMap(synonymFilterFilePath); // construct the synoymMap
	this->keepOriginFlag = synonymKeepOriginFlag;
}

void SynonymFilter::createMap(const std::string &synonymFilePath) {
	// using file path to create an ifstream object
	std::ifstream input(synonymFilePath.c_str());
	// Reads the map file line by line and fills the map
	std::string line;
	while (getline(input, line)) {
		/*
		 * for example we have "A=>B" in the file
		 * "A" is leftHandSide
		 * "B" is rightHandSide
		 */
		std::size_t index = line.find(this->synonymDelimiter);
		// if we don't have any synonymDelimeter in this line OR leftHandSide is empty, we should go to next line.
		if (index <= 0) {
			continue;
		}
		string leftHandSide = line.substr(0, index);
		string rightHandSide = line.substr(index + this->synonymDelimiter.length());

		/*
		 * This part will put the whole lefthandside into the map.
		 * It checks if it already exists or not.
		 * If the lefthandside is already there: only if it was prefix_only we will change it to prefix_and_complete, Otherwise, we won't touch it.
		 * If the lefthandside is not already there: it inserst it into the map.
		 */
		std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(leftHandSide);
		if (pos != this->synonymMap.end()) {
			if (this->synonymMap[leftHandSide].first == SYNONYM_PREFIX_ONLY) {
				ASSERT(this->synonymMap[leftHandSide].second == "");
				this->synonymMap[leftHandSide].first =  SYNONYM_PREFIX_AND_COMPLETE;
				this->synonymMap[leftHandSide].second =  rightHandSide;
			}
		} else {
			this->synonymMap.insert(make_pair(leftHandSide, make_pair(SYNONYM_COMPLETE_ONLY, rightHandSide)));
		}
		ASSERT(this->synonymMap[leftHandSide].first != SYNONYM_NOT_PREFIX_NOT_COMPLETE);

		/*
		 * Here will add the sub sequence of Tokens to the map.
		 * For example, if the lefthandside is "new york city", the whole string is already inserted intro the map.
		 * Now we should take care of "new york" and "new"
		 * In the while() loop, first "new york" will be added and then "new"
		 * For each of them, if they are already there and their flag is complete_only, we change them to the prefix_and_complete
		 * and if they are not there, we add them as prefix_only
		 */
		std::size_t found ;
		while (true) {
			found = leftHandSide.rfind(" ");
			if (found == std::string::npos) {
				break;
			}
			leftHandSide = leftHandSide.substr(0, found);
			std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(leftHandSide);
			if (pos != this->synonymMap.end()) {
				if (this->synonymMap[leftHandSide].first == SYNONYM_COMPLETE_ONLY) {
					ASSERT(this->synonymMap[leftHandSide].second != ""); // unless the righthandsde is empty in the synonym file
					this->synonymMap[leftHandSide].first =  SYNONYM_PREFIX_AND_COMPLETE;
				}
			} else {
				this->synonymMap.insert(make_pair(leftHandSide, make_pair(SYNONYM_PREFIX_ONLY, "")));
			}
			ASSERT(this->synonymMap[leftHandSide].first != SYNONYM_NOT_PREFIX_NOT_COMPLETE);
		}

	}
}


pair<SynonymTokenType, std::string> & SynonymFilter::getValuePairOf(const std::string &key) {
	std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(key);
	pair<SynonymTokenType, std::string> valuePair;
	if (pos != this->synonymMap.end()) {
		valuePair = make_pair(pos->second.first, pos->second.second);
	} else {
		valuePair = make_pair(SYNONYM_NOT_PREFIX_NOT_COMPLETE, "");
	}
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
			std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(tempToken);
			if (pos != this->synonymMap.end()) {
				if (this->synonymMap[tempToken].first != SYNONYM_PREFIX_ONLY) {
					if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
						for (int k = 0; k <=i; k++) {
							this->emitBuffer.push_back(this->tokenBuffer[k]);
						}
					}
					this->emitBuffer.push_back(this->synonymMap[tempToken].second);
					for (int k = i; k >= 0; k--) {
						this->tokenBuffer.erase(this->tokenBuffer.begin() + k);
						flag = true;
					}
					break;
				}
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
	utf8StringToCharTypeVector(this->emitBuffer[0], sharedToken->currentToken);
	// removing the first element
	this->emitBuffer.erase(this->emitBuffer.begin() + 0);
}

bool SynonymFilter::incrementToken() {
	while (true) {
		// if increment returns false
		if (!this->tokenOperator->incrementToken()) {
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
		charTypeVectorToUtf8String(sharedToken->currentToken, currentToken);
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
