/*
 * SimpleAnalyzer.cpp
 *
 *  Created on: 2013-5-18
 */

#include <sys/stat.h>
#include "SimpleAnalyzer.h"
#include "WhiteSpaceTokenizer.h"
#include "LowerCaseFilter.h"
#include "StemmerFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"
#include "util/Logger.h"
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

// create operator flow and link share pointer to the data
TokenStream * SimpleAnalyzer::createOperatorFlow() {
	TokenStream *tokenStream = new WhiteSpaceTokenizer();
	tokenStream = new LowerCaseFilter(tokenStream);

	if (this->stemmerType == ENABLE_STEMMER_NORMALIZER) {
		struct stat stResult;
		if(stat(this->stemmerFilePath.c_str(), &stResult) == 0) {
		    tokenStream = new StemmerFilter(tokenStream, this->stemmerFilePath);
		} else {
			this->stemmerType = DISABLE_STEMMER_NORMALIZER;
            Logger::error("The stemmer file %s is not valid. Please provide a valid file path." , this->stemmerFilePath.c_str());
		}
	}

	if (this->stopWordFilePath.compare("") != 0) {
		struct stat stResult;
		if(stat(this->stopWordFilePath.c_str(), &stResult) == 0) {
		    tokenStream = new StopFilter(tokenStream, this->stopWordFilePath);
		} else {
            Logger::error("The stop word file %s is not valid. Please provide a valid file path.", this->stopWordFilePath.c_str());
		}
	}

	if (this->synonymFilePath.compare("") != 0) {
		struct stat stResult;
		if(stat(this->synonymFilePath.c_str(), &stResult) == 0) {
		    tokenStream = new SynonymFilter(tokenStream, this->synonymFilePath, this->synonymKeepOriginFlag);
		} else {
            Logger::error("The synonym file %s. Please provide a valid file path.", this->synonymFilePath.c_str());
		}
	}

	this->tokenStreamContainer = tokenStream->tokenStreamContainer;
	return tokenStream;
}
SimpleAnalyzer::~SimpleAnalyzer() {
	// TODO Auto-generated destructor stub
}
}
}

