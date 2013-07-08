/*
 * StandardAnalyzer.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __STANDARDANALYZER_H__
#define __STANDARDANALYZER_H__

#include <vector>
#include <instantsearch/Analyzer.h>
#include "AnalyzerInternal.h"
#include "TokenOperator.h"

namespace srch2
{
namespace instantsearch
{

/*
 *  StandardAnalyzer use DELIMITER_TYPE to separate the LATIN_TYPE string and BOPOMOFO_TYPE.
 *  for OTHER_TYPE, each character is a token
 *  use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokened as "we" "went" "to" "学" and "校"
 */
class StandardAnalyzer:public AnalyzerInternal
{
public:
	StandardAnalyzer(const StemmerNormalizerType &stemNormType = NO_STEMMER_NORMALIZER, const std::string &recordAllowedSpecialCharacters = ""):AnalyzerInternal(stemNormType, recordAllowedSpecialCharacters)// Used by Analyzer.h
	{
		this->analyzerType = STANDARD_ANALYZER;
		this->tokenOperator = createOperatorFlow();
	}
	StandardAnalyzer(const StandardAnalyzer &standardAnalyzer):AnalyzerInternal(standardAnalyzer)
	{
		this->analyzerType = STANDARD_ANALYZER;
		this->tokenOperator = createOperatorFlow();
	}
	TokenOperator *createOperatorFlow();
	virtual ~StandardAnalyzer();
};

}}
#endif /* __STANDARDANALYZER_H__*/
