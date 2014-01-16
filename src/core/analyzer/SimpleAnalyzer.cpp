/*
 * SimpleAnalyzer.cpp
 *
 *  Created on: 2013-5-18
 */

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
        if (stopWords != NULL) {
            tokenStream = new StopFilter(tokenStream, stopWords);
        }
        if (synonyms != NULL) {
            tokenStream = new SynonymFilter(tokenStream, synonyms);
        }
        if (stemmer != NULL) {
            tokenStream = new StemmerFilter(tokenStream, stemmer);
        }

	return tokenStream;
}
SimpleAnalyzer::~SimpleAnalyzer() {
	// TODO Auto-generated destructor stub
}

AnalyzerType SimpleAnalyzer::getAnalyzerType() const
{
    return SIMPLE_ANALYZER;
}

}
}

