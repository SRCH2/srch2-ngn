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

namespace srch2{
namespace instantsearch{

ChineseAnalyzer::ChineseAnalyzer(const std::string &dictFile)
    :AnalyzerInternal(DISABLE_STEMMER_NORMALIZER,""),mDictPath(dictFile){
}

TokenStream* ChineseAnalyzer::createOperatorFlow(){
    TokenStream *tokenStream = new ChineseTokenizer(mDictPath);
    this->tokenStreamContainer = tokenStream->tokenStreamContainer;
    return tokenStream;
}

}
}
