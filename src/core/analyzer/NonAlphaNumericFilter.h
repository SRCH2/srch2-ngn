/*
 * NonAlphaNumericFilter.h
 *
 *  Created on: Nov 13, 2013
 *      Author: sbisht
 */

#ifndef __CORE_ANALYZER_NONALPHANUMERICFILTER_H__
#define __CORE_ANALYZER_NONALPHANUMERICFILTER_H__

#include "TokenFilter.h"
#include <set>
#include <string>
#include <queue>
#include "AnalyzerContainers.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class NonAlphaNumericFilter: public TokenFilter {
public:
	NonAlphaNumericFilter(TokenStream *tokenStream, const ProtectedWordsContainer *protectedWordsFilePath);
	void clearState();
	bool processToken();
	virtual ~NonAlphaNumericFilter();
	bool isProtectWord(const string& val)
        {
            if (protectedWordsContainer != NULL)
                return protectedWordsContainer->isProtected(val);
            return false;
        }

private:
	const ProtectedWordsContainer *protectedWordsContainer;
	// queue of <token | offset>
	// offset is the char position of token in the original string fetched from upstream.
	// e.g java-script =>  [ (java, 0) , (script, 5)]
	//     #tag => [(tag, 1)]
	queue< std::pair<vector<CharType>, short> > internalTokenBuffer;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* __CORE_ANALYZER_NONALPHANUMERICFILTER_H__ */
