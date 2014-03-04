//$Id$

#include "ChineseAnalyzer.h"
#include "ChineseTokenizer.h"
#include "LowerCaseFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"
#include "util/Logger.h"

namespace srch2{
namespace instantsearch{

ChineseAnalyzer::ChineseAnalyzer(const std::string &chineseDictionaryFile,
                                 const StopWordContainer *stopWords,
                                 const ProtectedWordsContainer *protectedWords,
                                 const SynonymContainer *synonyms,
                                 const std::string &delimiters)
    : AnalyzerInternal(NULL/*stemmer*/, stopWords, protectedWords, synonyms, delimiters),
      mDictFilePath(chineseDictionaryFile)
{
    this->analyzerType = CHINESE_ANALYZER;
}

ChineseAnalyzer::ChineseAnalyzer(const ChineseAnalyzer &analyzer)
    : AnalyzerInternal(analyzer), mDictFilePath(analyzer.mDictFilePath)
{
    this->analyzerType = CHINESE_ANALYZER;
}
  
AnalyzerType ChineseAnalyzer::getAnalyzerType() const
{
    return CHINESE_ANALYZER;
}

TokenStream* ChineseAnalyzer::createOperatorFlow(){
    TokenStream *tokenStream = new ChineseTokenizer(mDictFilePath);
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
