/*
 * NonAlphaNumericFilter.h
 *
 *  Created on: Nov 13, 2013
 *      Author: sbisht
 */

#ifndef XYZFILTER_H_
#define XYZFILTER_H_

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
	bool processToken();
	virtual ~NonAlphaNumericFilter();
	bool isProtectWord(const string& val) { return protectedWordsContainer.isProtected(val); }
private:
	ProtectedWordsContainer& protectedWordsContainer;
	queue<vector<CharType> > internalTokenBuffer;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* XYZFILTER_H_ */
