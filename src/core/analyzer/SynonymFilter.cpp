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
//#include <map>
#include <iostream>
#include <stdio.h>
#include <fstream>

using namespace std;

namespace srch2 {
namespace instantsearch {

SynonymFilter::SynonymFilter(TokenOperator * tokenOperator) :
		TokenFilter(tokenOperator) {
	this->sharedToken = tokenOperator->sharedToken; // copies the shared_ptr: sharedToken
	const std::string synonymFilePath =
			"/home/iman/srch2/bimaple-root/codebase/mario/branches/stemmer/src/analyzer/data/synonymFile.txt";
	this->createSynonymMap(synonymFilePath); // construct the synoymMap
}

void SynonymFilter::createSynonymMap(const std::string &synonymFilePath) {
	//  using file path to create an ifstream object
	std::ifstream input(synonymFilePath.c_str());
	//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
		cerr << "\nThe stop words list file could not be opened.\n";
		cerr << "The path is: " << synonymFilePath << endl;
		return;
	}
	//	Reads the map file line by line and fills the map
	std::string line;
	while (getline(input, line)) {
		/*
		 * for example we have "A=>B" in the file
		 * "A" is leftHandSide
		 * "B" is rightHandSide
		 */
		std::size_t index = line.find("=>");
		string leftHandSide = line.substr(0, index);
		string rightHandSide = line.substr(index + 2);
		// insert the "A" and "B" into the map
		this->synonymMap.insert(
				std::pair<string, string>(leftHandSide, rightHandSide));
	}
}

bool SynonymFilter::containsWord(const std::string &word) {
	// returns true if word exists, otherwise returns false
	return this->synonymMap.count(word) > 0;
}

bool SynonymFilter::isSubStringOfKey(const std::string &word) {
	// it is an iterator on all keys of the synonym map
	for (std::map<string, string>::iterator iter = this->synonymMap.begin();
			iter != this->synonymMap.end(); ++iter) {
		string key = iter->first;
		// if the word happens at the beginning of the a key, return true
		if (key.find(word) == 0) {
			return true;
		}
	}
	return false;
}

int SynonymFilter::countSubStringOfKey(const std::string &word) {

	// it is an iterator on all keys of the synonym map
	int count = 0;
	for (std::map<string, string>::iterator iter = this->synonymMap.begin();
			iter != this->synonymMap.end(); ++iter) {
		string key = iter->first;
		// if the word happens at the beginning of the a key, return true
		if (key.find(word) == 0) {
			count++;
		}
	}
	return count;
}

string SynonymFilter::getValueOf(const std::string &subWord) {
	std::map<string, string>::const_iterator pos = this->synonymMap.find(
			subWord);
	if (pos != this->synonymMap.end()) {
		return pos->second;
	}
	return NULL;
}

string SynonymFilter::getKeyOf(const std::string &subWord) {
	std::map<std::string, std::string>::iterator iter;
	for (iter = this->synonymMap.begin(); iter != this->synonymMap.end();
			iter++) {
		if (iter->first.find(subWord) == 0) {
			return iter->first;
		}
	}
	return NULL;
}

vector<std::string> SynonymFilter::getSynonymOfBuffered() {
	std::vector<std::string> result;
	bool flag;
	while (true) {
		if (this->tokenBuffer.size() == 0) {
			return result;
		}
		flag = false;
		for (int i = this->tokenBuffer.size() - 1; i >= 0; i--) {
			std::string tempToken = "";
			for (int j = 0; j <= i; j++) {
				tempToken += this->tokenBuffer[j] + " ";
			}
			tempToken = tempToken.substr(0, tempToken.length() - 1);
			std::map<string, string>::const_iterator pos =
					this->synonymMap.find(tempToken);
			if (pos != this->synonymMap.end()) {
				result.push_back(this->getValueOf(tempToken));
				for (int k = i; k >= 0; k--) {
					this->tokenBuffer.erase(this->tokenBuffer.begin() + k);
					flag = true;
				}
				break;
			}
		}
		if (!flag) {
			result.push_back(this->tokenBuffer[0]);
			this->tokenBuffer.erase(this->tokenBuffer.begin());
		}
	}
}

bool SynonymFilter::incrementToken() {
	while (true) {
		if (!this->tokenOperator->incrementToken()) {
			if (this->tokenBuffer.empty()){
				return false;
			} else {
				vector<string> tempResult = getSynonymOfBuffered();
				std::string previousTokens = "";
				for (int ii = 0; ii < tempResult.size(); ii++) {
					previousTokens += tempResult[ii] + " ";
				}
				previousTokens = previousTokens.substr(0, previousTokens.length()-1);
				utf8StringToCharTypeVector(previousTokens, sharedToken->currentToken);
				this->tokenBuffer.clear();
				return true;// TODO: false?
			}
		}
		if (this->tokenBuffer.empty()) {
			std::string currentToken = "";
			// converts the charType to string
			charTypeVectorToUtf8String(sharedToken->currentToken, currentToken);
//			cout << "current token:  " << currentToken << endl;
			if (this->countSubStringOfKey(currentToken) == 0) {
				return true;
			} else if (this->countSubStringOfKey(currentToken) == 1) {

				std::string key = this->getKeyOf(currentToken);
				// if two string are the same, it means that we have "A=>B" rule, not "A B=>C"
				if (key.compare(currentToken) == 0) {
					std::string value = this->getValueOf(currentToken);
					std::vector<CharType> tempToken;
					utf8StringToCharTypeVector(value, tempToken);
					sharedToken->currentToken = tempToken;
					return true;
				} else {
					this->tokenBuffer.push_back(currentToken);
				}
			} else {
				this->tokenBuffer.push_back(currentToken);
			}
		} else {
			std::string currentToken = "";
			// converts the charType to string
			charTypeVectorToUtf8String(sharedToken->currentToken, currentToken);
			// this will append all previous tokens in the tokenBuffer
			string previousTokens = "";
			for (int i = 0; i < this->tokenBuffer.size(); i++) {
				previousTokens = previousTokens + this->tokenBuffer[i] + " ";
			}
			if (this->countSubStringOfKey(previousTokens + currentToken) == 0) {
				// TODO: one by one, it is wrong now
				vector<string> tempResult = getSynonymOfBuffered();
				previousTokens = "";
				for (int ii = 0; ii < tempResult.size(); ii++) {
					previousTokens += tempResult[ii] + " ";
				}
				if (this->countSubStringOfKey(currentToken) == 0) {
					previousTokens += currentToken;
				} else if (this->countSubStringOfKey(currentToken) == 1) {
//					previousTokens += getValueOf(currentToken);
					this->sharedToken->offset = this->sharedToken->offset - currentToken.length() -1;
					previousTokens = previousTokens.substr(0, previousTokens.length() - 1);
				} else {
					previousTokens = previousTokens.substr(0,
							previousTokens.length() - 1);
					this->tokenBuffer.push_back(currentToken);
				}
				utf8StringToCharTypeVector(previousTokens,
						sharedToken->currentToken);
//				this->tokenBuffer.clear();
				return true;
			} else if (this->countSubStringOfKey(previousTokens + currentToken)
					== 1) {
				std::string key = this->getKeyOf(previousTokens);
				if (key.compare(previousTokens + currentToken) == 0) {
					std::string value = this->getValueOf(key);
					utf8StringToCharTypeVector(value,
							sharedToken->currentToken);
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
}

SynonymFilter::~SynonymFilter() {
}

}
}
