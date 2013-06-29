/*
 * StopFilter.h
 *
 *  Created on: Jun 21, 2013
 *      Author: iman
 */

#ifndef STOPFILTER_H_
#define STOPFILTER_H_

#include <iostream>
#include <string>
#include <vector>

#include "TokenOperator.h"
#include "TokenFilter.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class StopFilter: public TokenFilter {
public:
	/*
	 * Constructor of stop words.
	 * Set sharedToken and create the dictionary list
	 */
	StopFilter(TokenOperator *tokenOperator);

	/*
	 * IncrementToken() is a virtual function of class TokenOperator. Here we have to implement it. It goes on all tokens.
	 * */
	bool incrementToken();

	virtual ~StopFilter();

private:

	/*
	 * list of stop words
	 */
	std::vector<std::string> stopWordsVector;

	/*
	 *  input: a token
	 *  Checks if the token is a stop word or not
	 */
	bool isStopWord(const std::string &token) const;

	/*
	 *  creates the list of stop words.
	 */
	void createStopWordList(const std::string &indexDirectory);

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & stopWordsVector;
	}
};

}
}

#endif /* STOPFILTER_H_ */
