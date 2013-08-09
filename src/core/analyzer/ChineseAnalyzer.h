/*
 * =====================================================================================
 *
 *       Filename:  ChineseAnalyzer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/2013 06:20:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */
#ifndef __CORE_ANALYZER__CHINESEANALYZER_H__
#define __CORE_ANALYZER__CHINESEANALYZER_H__

#include "AnalyzerInternal.h"
#include "TokenStream.h"

namespace srch2{
namespace instantsearch{

class ChineseAnalyzer: public AnalyzerInternal{
public:
    ChineseAnalyzer(const std::string &dictFile); 
    TokenStream *createOperatorFlow();
private:
    const std::string &mDictPath;
};

}
}
#endif



