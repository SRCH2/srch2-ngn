/*
 * =====================================================================================
 *
 *       Filename:  ChineseAnalyzer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/10/2013 11:20:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jianfeng (), jianfengjia@srch2.com
 *   Organization:  SRCH2.com
 *
 * =====================================================================================
 */

#include "ChineseAnalyzer.h"
#include "ChineseTokenizer.h"
#include "LowerCaseFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"

namespace srch2{
namespace instantsearch{

ChineseAnalyzer::ChineseAnalyzer(const std::string &dictFile,
            const std::string &recordAllowedSpecialCharacters ,
            const std::string &stopWordFilePath ,
            const std::string &synonymFilePath ,
            const SynonymKeepOriginFlag &synonymKeepOriginFlag 
            )
    :AnalyzerInternal(DISABLE_STEMMER_NORMALIZER,
            recordAllowedSpecialCharacters,
            "", // Chinese do not have stemmer problem
            stopWordFilePath,
            synonymFilePath,
            synonymKeepOriginFlag), mDictPath(dictFile)
{}
  
TokenStream* ChineseAnalyzer::createOperatorFlow(){
    TokenStream *tokenStream = new ChineseTokenizer(mDictPath);
    tokenStream = new LowerCaseFilter(tokenStream);

	if (this->stopWordFilePath.compare("") != 0) {
        // The file os error should be solved by exception
		tokenStream = new StopFilter(tokenStream, this->stopWordFilePath);
	}

	if (this->synonymFilePath.compare("") != 0) {
        // The file os error should be solved by exception
		tokenStream = new SynonymFilter(tokenStream, this->synonymFilePath, this->synonymKeepOriginFlag);
	}

    this->tokenStreamContainer = tokenStream->tokenStreamContainer;
    return tokenStream;
}

}
}
