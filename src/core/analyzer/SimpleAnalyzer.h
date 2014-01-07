/*
 * SimpleAnalyzer.h
 *
 *  Created on: 2013-5-18
 */
//This class will tokenize the words according to whitespace character .
#ifndef __CORE_ANALYZER_SIMPLEANALYZER_H__
#define __CORE_ANALYZER_SIMPLEANALYZER_H__

#include "AnalyzerInternal.h"
#include "TokenStream.h"

namespace srch2 {
namespace instantsearch {

/*
 *  SimpleAnalyzer use DELIMITER_TYPE to separate the string and use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokenized as "we" "went" "to" and "学校"
 */
class SimpleAnalyzer: public AnalyzerInternal {
public:

    SimpleAnalyzer(const StemmerContainer *stemmer,
                   const StopWordContainer *stopWords,
                   const ProtectedWordsContainer *protectedWords,
                   const SynonymContainer *synonyms,
                   const std::string &allowedRecordSpecialCharacters) :
        AnalyzerInternal(stemmer, stopWords, protectedWords, synonyms, allowedRecordSpecialCharacters)
        {
            this->analyzerType = SIMPLE_ANALYZER;
        }

	SimpleAnalyzer(const SimpleAnalyzer &simpleAnalyzer) :
			AnalyzerInternal(simpleAnalyzer) {
		this->analyzerType = SIMPLE_ANALYZER;
	}
	TokenStream *createOperatorFlow();
	virtual ~SimpleAnalyzer();

    AnalyzerType getAnalyzerType() const;
};
}
}
#endif /* __CORE_ANALYZER__SIMPLEANALYZER_H__ */
