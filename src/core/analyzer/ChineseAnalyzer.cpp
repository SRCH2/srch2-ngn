//$Id$

#include "ChineseAnalyzer.h"
#include "ChineseTokenizer.h"
#include "LowerCaseFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"
#include "util/Logger.h"

namespace srch2{
namespace instantsearch{

ChineseAnalyzer::ChineseAnalyzer(const ChineseDictionaryContainer* chineseDictionaryContainer,
                                 const StopWordContainer *stopWords,
                                 const ProtectedWordsContainer *protectedWords,
                                 const SynonymContainer *synonyms,
                                 const std::string &delimiters)
    : AnalyzerInternal(NULL/*stemmer*/, stopWords, protectedWords, synonyms, delimiters),
      mChineseDictionaryContainer(chineseDictionaryContainer)
{
	this->tokenStream = NULL;
    this->analyzerType = CHINESE_ANALYZER;
}

AnalyzerType ChineseAnalyzer::getAnalyzerType() const
{
    return CHINESE_ANALYZER;
}

TokenStream* ChineseAnalyzer::createOperatorFlow(){
    TokenStream *tokenStream = new ChineseTokenizer(mChineseDictionaryContainer);
    tokenStream = new LowerCaseFilter(tokenStream);

    if (this->stopWords != NULL) {
        tokenStream = new StopFilter(tokenStream, this->stopWords);
    }

    if (this->synonyms != NULL) {
        tokenStream = new SynonymFilter(tokenStream, this->synonyms);
    }

    return tokenStream;
}

}
}
