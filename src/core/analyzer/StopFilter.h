/*
 * StopFilter.h
 *
 *  Created on: Jun 21, 2013
 *      Author: iman
 */

#ifndef __CORE_ANALYZER_STOPFILTER_H__
#define __CORE_ANALYZER_STOPFILTER_H__

#include <iostream>
#include <string>
#include <vector>

#include "TokenStream.h"
#include "TokenFilter.h"

using namespace std;

namespace srch2 {
namespace instantsearch {
class StopWordContainer;

class StopFilter: public TokenFilter {
public:
	/*
	 * Constructor of stop words.
	 * Set sharedToken and create the dictionary list
	 */
	StopFilter(TokenStream *tokenStream, std::string &stopFilterFilePath);

	/*
	 * IncrementToken() is a virtual function of class TokenOperator. Here we have to implement it. It goes on all tokens.
	 * */
	bool processToken();

	virtual ~StopFilter();

private:

	StopWordContainer& stopWordsContainer;

	/*
	 *  input: a token
	 *  Checks if the token is a stop word or not
	 */
	bool isStopWord(const std::string &token) const;


};

}
}

#endif /* __CORE_ANALYZER__STOPFILTER_H__ */
