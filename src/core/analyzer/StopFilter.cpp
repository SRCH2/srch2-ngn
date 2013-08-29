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
#include <fstream>
#include "util/Logger.h"
#include "analyzer/AnalyzerContainers.h"

using namespace std;
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

StopFilter::StopFilter(TokenStream *tokenStream,
		std::string &stopFilterFilePath) : TokenFilter(tokenStream),
				stopWordsContainer(StopWordContainer::getInstance()) {
	this->tokenStreamContainer = tokenStream->tokenStreamContainer; // copies the shared_ptr: sharedToken
}

 /*
  * Checks if the input token is in the stop words list or not
 * */
bool StopFilter::isStopWord(const std::string &token) const {
	// returns true if the given token is a stop word, else it reaturns false
	return stopWordsContainer.contains(token);
}

bool StopFilter::processToken() {
	while (true) {
		if (!this->tokenStream->processToken()) {
			return false;
		}
		std::string currentToken = "";
		// converts the charType to string
		charTypeVectorToUtf8String(tokenStreamContainer->currentToken, currentToken);
		// returns true if the currentToken is one of the stop words.
		if (!this->isStopWord(currentToken)) {
			return true;
		}
	}
	return false; // The function will never reach this point. This return is for avoiding the warning.
}

StopFilter::~StopFilter() {
}

}}
