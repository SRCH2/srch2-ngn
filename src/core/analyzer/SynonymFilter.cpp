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

using namespace std;

namespace srch2 {
namespace instantsearch {

SynonymFilter::SynonymFilter(TokenOperator * tokenOperator,
		const std::string &synonymFilterFilePath,
		const SynonymKeepOriginFlag &synonymKeepOriginFlag) :
		TokenFilter(tokenOperator), synonymDelimiter("=>") {
	this->sharedToken = tokenOperator->sharedToken; // copies the shared_ptr: sharedToken
	this->createMaps(synonymFilterFilePath); // construct the synoymMap
	this->keepOriginFlag = synonymKeepOriginFlag;
}

void SynonymFilter::createMaps(const std::string &synonymFilePath) {
	//  using file path to create an ifstream object
	std::ifstream input(synonymFilePath.c_str());
	//	Reads the map file line by line and fills the map
	std::string line;
	while (getline(input, line)) {
		/*
		 * for example we have "A=>B" in the file
		 * "A" is leftHandSide
		 * "B" is rightHandSide
		 */
		std::size_t index = line.find(this->synonymDelimiter);
		// if we don't have any synonymDelimeter in this line OR leftHandSide is empty, we should go to next line.
		if (index <= 0) { //TODO: write a message in logger
			continue;
		}
		string leftHandSide = line.substr(0, index);
		string rightHandSide = line.substr(index + 2);
		// insert the "A" and "B" into the map
		this->synonymMap.insert(
				std::pair<string, string>(leftHandSide, rightHandSide));

		// adding leftHandSide to the prefixMap
		std::size_t found ;
		while (true) {
			std::map<string, bool>::const_iterator pos = this->prefixMap.find(leftHandSide);
			if (pos != this->prefixMap.end()) {
				this->prefixMap[leftHandSide] =  waitForNextToken;
			} else {
				this->prefixMap.insert(std::pair<string, bool>(leftHandSide, checkExistingTokens));
			}

			found = leftHandSide.rfind(" ");
			if (found == std::string::npos) {
				break;
			}
			leftHandSide = leftHandSide.substr(0, found);
		}

	}
}


int SynonymFilter::numberOfKeysHavingTokenAsPrefix(const std::string &prefixToken) {
	std::map<string, bool>::const_iterator pos = this->prefixMap.find(prefixToken);
	if (pos != this->prefixMap.end()) {
		if (pos->second == waitForNextToken) {
			return  2;
		}
		return 1 ;
	} else {
		return 0;
	}
}

const string SynonymFilter::getSynonymOf(const std::string &word) {
	// finds the subWord in the map
	std::map<string, string>::const_iterator pos = this->synonymMap.find(word);
	if (pos != this->synonymMap.end()) {
		return pos->second;
	}
	return "";
}

bool SynonymFilter::isCurrentTokenExistInSynonymMap(const std::string &key) {
	std::map<string, string>::const_iterator pos = this->synonymMap.find(key);
	if (pos != this->synonymMap.end()) {
		return true;
	}
	return false;
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
			std::map<string, string>::const_iterator pos = this->synonymMap.find(tempToken);
			if (pos != this->synonymMap.end()) {
				if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
					for (int k = 0; k <=i; k++) {
						this->emitBuffer.push_back(this->tokenBuffer[k]);
					}
				}
				this->emitBuffer.push_back(this->getSynonymOf(tempToken));
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
		int numberOfKeysHavingCurrentTokenAsTheirPrefix =
				this->numberOfKeysHavingTokenAsPrefix(currentToken);

		// if increment returns true
		if (this->tokenBuffer.empty()) {
			// if the currentToken is not prefix of any of the keys in the map
			// if it is zero we have to emit this token (because there is not any synonym match for it)
			if (numberOfKeysHavingCurrentTokenAsTheirPrefix == 0) {
				this->emitBuffer.push_back(currentToken);
				this->emitCurrentToken();
				return true;
			// if there is one synonym match for the new token
			} else if (numberOfKeysHavingCurrentTokenAsTheirPrefix == 1) {
				if (this->isCurrentTokenExistInSynonymMap(currentToken)) {
					if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
						this->emitBuffer.push_back(currentToken); // this is for adding the original tokens.
					}
					this->emitBuffer.push_back(this->getSynonymOf(currentToken));
					this->emitCurrentToken();
					return true;
				} else {
					this->tokenBuffer.push_back(currentToken);
				}
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

			int numberOfKeysHavingPreviousFollowedByCurrentTokenAsTheirPrefix =
					this->numberOfKeysHavingTokenAsPrefix(previousTokens + currentToken);
			// if there is NOT any match for the elements in the buffer that followed by current token

			if ( numberOfKeysHavingPreviousFollowedByCurrentTokenAsTheirPrefix == 0) {
				pushSynonymsOfExistingTokensInEmitBuffer();
				if (numberOfKeysHavingCurrentTokenAsTheirPrefix == 0) {
					this->emitBuffer.push_back(currentToken);
				} else if (numberOfKeysHavingCurrentTokenAsTheirPrefix == 1) {
					if (this->isCurrentTokenExistInSynonymMap(currentToken)) {
						if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
							this->emitBuffer.push_back(currentToken); // this is for adding the original tokens.
						}
						this->emitBuffer.push_back(this->getSynonymOf(currentToken));
					} else {
						this->tokenBuffer.push_back(currentToken);
					}
				} else {
					this->tokenBuffer.push_back(currentToken);
				}
				this->emitCurrentToken();
				return true;
				// if there is ONE match for the elements in the buffer that followed by current token
			} else if (numberOfKeysHavingPreviousFollowedByCurrentTokenAsTheirPrefix == 1) {
				/*
				* Now we should check if they have a complete match OR
				* again this combination of buffer and current token is a prefix of a key in the synonym or not
				* if it is complete we should just replace them with the synonym
				* if it is still not complete, we have to push the current token into the buffer and move on.
				*/
				if (this->isCurrentTokenExistInSynonymMap(previousTokens + currentToken)) {
					if (this->keepOriginFlag == SYNONYM_KEEP_ORIGIN) { // checks the flag of Keeping origin word
						for (int i =0; i < tokenBuffer.size(); i++) {
							this->emitBuffer.push_back(tokenBuffer[i]);
						}
						this->emitBuffer.push_back(currentToken);
					}
					this->emitBuffer.push_back(this->getSynonymOf(previousTokens + currentToken));
					this->emitCurrentToken();
					this->tokenBuffer.clear();
					return true;
				} else {
					this->tokenBuffer.push_back(currentToken);
				}
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
