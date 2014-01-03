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
	NonAlphaNumericFilter(TokenStream *tokenStream);
	void clearState();
	bool processToken();
	virtual ~NonAlphaNumericFilter();
	bool isProtectWord(const string& val) { return protectedWordsContainer.isProtected(val); }
private:
	ProtectedWordsContainer& protectedWordsContainer;
	queue<vector<CharType> > internalTokenBuffer;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* __CORE_ANALYZER_NONALPHANUMERICFILTER_H__ */
