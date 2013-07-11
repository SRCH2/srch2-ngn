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

namespace srch2 {
namespace instantsearch {

// create operator flow and link share pointer to the data
TokenOperator * SimpleAnalyzer::createOperatorFlow() {
	TokenOperator *tokenOperator = new WhiteSpaceTokenizer();
	tokenOperator = new LowerCaseFilter(tokenOperator);

	if (this->stemmerType == ENABLE_STEMMER_NORMALIZER) {
		struct stat stResult;
		if(stat(this->stemmerFilePath.c_str(), &stResult) == 0) {
			tokenOperator = new StemmerFilter(tokenOperator, this->stemmerFilePath);
		} else {
			this->stemmerType = DISABLE_STEMMER_NORMALIZER;
			cerr << "The stemmer file (path) that is provided is not valid. Please provide a valid file path." << endl;
		}
	}

	if (this->stopWordFilePath.compare("") != 0) {
		struct stat stResult;
		if(stat(this->stopWordFilePath.c_str(), &stResult) == 0) {
			tokenOperator = new StopFilter(tokenOperator, this->stopWordFilePath);
		} else {
			cerr << "The stop word file (path) that is provided is not valid. Please provide a valid file path." << endl;
		}
	}

	if (this->synonymFilePath.compare("") != 0) {
		struct stat stResult;
		if(stat(this->synonymFilePath.c_str(), &stResult) == 0) {
			tokenOperator = new SynonymFilter(tokenOperator, this->synonymFilePath);
		} else {
			cerr << "The synonym file (path) that is provided is not valid. Please provide a valid file path." << endl;
		}
	}

	this->sharedToken = tokenOperator->sharedToken;
	return tokenOperator;
}
SimpleAnalyzer::~SimpleAnalyzer() {
	// TODO Auto-generated destructor stub
}
}
}

