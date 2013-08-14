//$Id$

#include "ChineseAnalyzer.h"
#include "ChineseTokenizer.h"
#include "LowerCaseFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"

namespace srch2{
namespace instantsearch{

ChineseAnalyzer::ChineseAnalyzer(const std::string &chineseDictionaryFile, const std::string &recordAllowedSpecialCharacters ,
            const std::string &stopWordFilePath ,
            const std::string &synonymFilePath ,
            const SynonymKeepOriginFlag &synonymKeepOriginFlag 
            )
    :AnalyzerInternal(DISABLE_STEMMER_NORMALIZER,
            recordAllowedSpecialCharacters,
            "", // The Chinese language does not need stemming as English
            stopWordFilePath,
            synonymFilePath,
            synonymKeepOriginFlag), mDictFilePath(chineseDictionaryFile)
{}
  
TokenStream* ChineseAnalyzer::createOperatorFlow(){
    TokenStream *tokenStream = new ChineseTokenizer(mDictFilePath);
    tokenStream = new LowerCaseFilter(tokenStream);

    if (this->stopWordFilePath.compare("") != 0) {
        //TODO: The file os error should be solved by exception
        tokenStream = new StopFilter(tokenStream, this->stopWordFilePath);
    }

    if (this->synonymFilePath.compare("") != 0) {
        //TODO: The file os error should be solved by exception
        tokenStream = new SynonymFilter(tokenStream, this->synonymFilePath, this->synonymKeepOriginFlag);
    }

    this->tokenStreamContainer = tokenStream->tokenStreamContainer;
    return tokenStream;
}

}
}
