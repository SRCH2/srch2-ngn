
/*
 * StopFilter.cpp
 *
 *  Created on: Jun 21, 2013
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
                       const StopWordContainer *_stopWordsContainer) :
    TokenFilter(tokenStream),
    stopWordsContainer(_stopWordsContainer)
{
	this->tokenStreamContainer = tokenStream->tokenStreamContainer; // copies the shared_ptr: sharedToken
}

 /*
  * Checks if the input token is in the stop words list or not
 * */
bool StopFilter::isStopWord(const std::string &token) const {
	// returns true if the given token is a stop word, else it reaturns false
	return stopWordsContainer->contains(token);
}

bool StopFilter::processToken() {
	while (true) {
		if (!this->tokenStream->processToken()) {
			return false;
		}
		std::string currentToken = "";
		// converts the charType to string
		charTypeVectorToUtf8String(tokenStreamContainer->currentToken, currentToken);
		// returns true if the currentToken is NOT the stop word. If it is a stop word then check
		// whether it is a prefix. For a prefix, stop filter is not applied.
		if (!this->isStopWord(currentToken) || this->tokenStreamContainer->isPrefix) {
			return true;
		}
	}
	return false; // The function will never reach this point. This return is for avoiding the warning.
}

StopFilter::~StopFilter() {
}

}}
