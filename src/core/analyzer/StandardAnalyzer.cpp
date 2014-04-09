/*
 * StandardAnalyzer.cpp
 *
 *  Created on: 2013-5-17
 */

#include "StandardAnalyzer.h"
#include "StandardTokenizer.h"
#include "LowerCaseFilter.h"
#include "StemmerFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"
#include "NonAlphaNumericFilter.h"
#include "util/Logger.h"

using srch2::util::Logger;
namespace srch2 {
namespace instantsearch {

StandardAnalyzer::StandardAnalyzer(const StemmerContainer *stemmer,
                                   const StopWordContainer *stopWords,
                                   const ProtectedWordsContainer *protectedWords,
                                   const SynonymContainer *synonyms,
                                   const std::string &allowedRecordSpecialCharacters) :
    AnalyzerInternal(stemmer, stopWords, protectedWords, synonyms, allowedRecordSpecialCharacters)
{
    this->tokenStream = NULL;
    this->analyzerType = STANDARD_ANALYZER;
}

AnalyzerType StandardAnalyzer::getAnalyzerType() const
{
    return STANDARD_ANALYZER;
}

// create operator flow and link share pointer to the data
TokenStream * StandardAnalyzer::createOperatorFlow() {
	TokenStream *tokenStream = new StandardTokenizer();
	tokenStream = new LowerCaseFilter(tokenStream);
        tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);

        tokenStream = new NonAlphaNumericFilter(tokenStream, protectedWords);
        tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);

        if (stopWords != NULL) {
            tokenStream = new StopFilter(tokenStream, stopWords);
            tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
        }

	if (this->synonyms != NULL) {
            tokenStream = new SynonymFilter(tokenStream, synonyms);
            tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
	}

	if (this->stemmer != NULL) {
            tokenStream = new StemmerFilter(tokenStream, stemmer);
            tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
	}

	return tokenStream;
}

StandardAnalyzer::~StandardAnalyzer() {
	delete this->tokenStream;
}

}
}
