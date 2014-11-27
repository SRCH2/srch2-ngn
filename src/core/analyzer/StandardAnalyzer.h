/*
 * StandardAnalyzer.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER_STANDARDANALYZER_H__
#define __CORE_ANALYZER_STANDARDANALYZER_H__

#include <vector>
#include <instantsearch/Analyzer.h>
#include "AnalyzerInternal.h"
#include "TokenStream.h"

namespace srch2 {
namespace instantsearch {

class StemmerContainer;
class StopWordContainer;
class SynonymContainer;
class ProtectedWordsContainer;

/*
 *  StandardAnalyzer use DELIMITER_TYPE to separate the LATIN_TYPE string and BOPOMOFO_TYPE.
 *  for OTHER_TYPE, each character is a token
 *  use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokened as "we" "went" "to" "学" and "校"
 */
class StandardAnalyzer: public AnalyzerInternal {
public:
    StandardAnalyzer(const StemmerContainer *stemmer,
                     const StopWordContainer *stopWords,
                     const ProtectedWordsContainer *protectedWords,
                     const SynonymContainer *synonyms,
                     const std::string &allowedRecordSpecialCharacters);

	StandardAnalyzer(const StandardAnalyzer &standardAnalyzer) :
			AnalyzerInternal(standardAnalyzer) {
		this->analyzerType = STANDARD_ANALYZER;
	}

	TokenStream *createOperatorFlow();
	virtual ~StandardAnalyzer();

    AnalyzerType getAnalyzerType() const;

 protected:

};

}
}
#endif /* __CORE_ANALYZER__STANDARDANALYZER_H__*/
