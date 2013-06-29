/*
 * SimpleAnalyzer.h
 *
 *  Created on: 2013-5-18
 */
//This class will token the words according to whitespace character .
#ifndef __SIMPLEANALYZER_H__
#define __SIMPLEANALYZER_H__

#include "AnalyzerInternal.h"
#include "TokenOperator.h"

namespace srch2 {
namespace instantsearch {

/*
 *  SimpleAnalyzer use DELIMITER_TYPE to separate the string and use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokenized as "we" "went" "to" and "学校"
 */
class SimpleAnalyzer: public AnalyzerInternal {
public:
	SimpleAnalyzer(const StemmerNormalizerFlagType &stemmerFlag =DISABLE_STEMMER_NORMALIZER,
			const std::string &recordAllowedSpecialCharacters = "")
			: AnalyzerInternal(stemmerFlag, recordAllowedSpecialCharacters) {
		this->analyzerType = SIMPLE_ANALYZER;
		this->stemmerType = stemmerFlag;
		this->tokenOperator = createOperatorFlow();
	}
	SimpleAnalyzer(const SimpleAnalyzer &simpleAnalyzer) :
			AnalyzerInternal(simpleAnalyzer) {
		this->analyzerType = SIMPLE_ANALYZER;
		this->tokenOperator = createOperatorFlow();
	}
	TokenOperator *createOperatorFlow();
	virtual ~SimpleAnalyzer();
};
}
}
#endif /* __SIMPLEANALYZER_H__ */
