// $Id: StopFilter.cpp 3074 2013-20-06 22:26:36Z iman $

/*
 * StopFilter.cpp
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


#include "StopFilter.h"


#include <string.h>
#include <iostream>
#include <stdio.h>
#include <fstream>



using namespace std;

namespace srch2
{
namespace instantsearch
{


StopFilter::StopFilter(TokenOperator *tokenOperator):TokenFilter(tokenOperator)
{
	const std::string stopWordsFilePath = "/home/iman/srch2/bimaple-root/codebase/mario/branches/stemmer/src/analyzer/data/stopWordsFile.txt";
	this->sharedToken = tokenOperator->sharedToken;// copies the shared_ptr: sharedToken
    this->createStopWordList(stopWordsFilePath);// construct the stopWordDictionary
}

 /*
  * Checks if the input token is in the stop words list or not
  * */
 bool StopFilter::isStopWord(const std::string &token) const
 {
	 // returns true if the given token is a stop word, else it reaturns false
	 return (std::find(this->stopWordsVector.begin(), this->stopWordsVector.end(), token) != this->stopWordsVector.end());
 }

 bool StopFilter::incrementToken()
 {
	 while (true){
		 if (!this->tokenOperator->incrementToken()){
			 return false;
		 }
		 std::string currentToken = "";
		 // converts the charType to string
		 charTypeVectorToUtf8String(sharedToken->currentToken, currentToken);
		 if(!this->isStopWord(currentToken)){
			 return true;
		 }
	 }
//	 return true;

//	if(this->tokenOperator->incrementToken())
//	{
//		// TODO: remove "charTypeVectorToUtf8String()"
//		std::string currentToken = "";
//		// converts the charType to string
//		charTypeVectorToUtf8String(sharedToken->currentToken, currentToken);
//		// calls the stemToken to stem
//		if (this->isStopWord(currentToken))		{
//			utf8StringToCharTypeVector("", sharedToken->currentToken);
//		}
//		return true;
//	}else{
//		 return false;
//	}
 }

 StopFilter::~StopFilter()
 {
 }


 void StopFilter::createStopWordList(const std::string &stopWordsFilePath)
 {
	 std::string str;
	 //  using file path to create an ifstream object
	 std::ifstream input(stopWordsFilePath.c_str());
	 //  If the file path is OK, it will be passed, else this if will run and the error will be shown
	 if(input.fail()){
		 cerr << "\nThe stop words list file could not be opened.\n";
		 cerr << "The path is: " << stopWordsFilePath << endl;
		 return;
	 }
	 //	Reads the stop word files line by line and fills the vector
	 while (getline(input,str)){
		 this->stopWordsVector.push_back(str);
	 }
 }



}}
