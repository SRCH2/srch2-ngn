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
            const std::string &recordAllowedSpecialCharacters = "",
            const std::string &stopWordFilePath = "",
            const std::string &synonymFilePath = "",
            const SynonymKeepOriginFlag &synonymKeepOriginFlag = SYNONYM_KEEP_ORIGIN
            ); 
    TokenStream *createOperatorFlow();
private:
    const std::string &mDictFilePath;
};

}
}
#endif



