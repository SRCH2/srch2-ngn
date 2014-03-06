//$Id$
#ifndef __CORE_ANALYZER_CHINESEANALYZER_H__
#define __CORE_ANALYZER_CHINESEANALYZER_H__

#include "AnalyzerInternal.h"
#include "TokenStream.h"

namespace srch2{
namespace instantsearch{

class ChineseAnalyzer: public AnalyzerInternal{
public:
    // We do not need the English stemmer for the Chinese analyzer
    ChineseAnalyzer(const std::string &dictFile,
                    const StopWordContainer *stopWords,
                    const ProtectedWordsContainer *protectedWords,
                    const SynonymContainer *synonyms,
                    const std::string &delimiters);
    ChineseAnalyzer(const ChineseAnalyzer &analyzer);

    TokenStream *createOperatorFlow();

    AnalyzerType getAnalyzerType() const;

private:
    const std::string mDictFilePath;
};

}
}
#endif



