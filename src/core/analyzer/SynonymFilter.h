// $Id: StopFilter.cpp 3074 2013-21-06 22:26:36Z iman $

/*
 * SynonymFilter.h
 *
 *  Created on: Jun 21, 2013
 *      Author: iman
 */

#ifndef __CORE_ANALYZER__SYNONYMFILTER_H_ANALYZER__
#define __CORE_ANALYZER__SYNONYMFILTER_H_ANALYZER__

#include <string>
#include <vector>
#include <map>

#include "TokenOperator.h"
#include "TokenFilter.h"
#include "instantsearch/Analyzer.h"

#define waitForNextToken true
#define checkExistingTokens false


using namespace std;

namespace srch2 {
namespace instantsearch {

class SynonymFilter: public TokenFilter {
public:
	/*
	 * Constructor of synonym filter.
	 * Sets sharedToken.
	 */
	SynonymFilter(TokenOperator *tokenOperator,
			const std::string &synonymFilterFilePath,
			const SynonymKeepOriginFlag &synonymKeepOriginFlag);

	/*
	 * IncrementToken() is a virtual function of class TokenOperator.
	 * Here we have to implement it. It goes on all tokens.
	 * */
	bool incrementToken();

	virtual ~SynonymFilter();

private:

	/*
	 * It is about keeping the original keyword or not
	 */
	srch2::instantsearch::SynonymKeepOriginFlag keepOriginFlag;

	const std::string synonymDelimiter;

	/*
	 * synonymMap is the map of synonyms
	 * If we have following rules:
	 * A => B
	 * C D => E
	 * F, G => H
	 * synonymMap will contains following records:
	 * A => B
	 * C D => E
	 * F => H
	 * G => H
	 */
	map<std::string, std::string> synonymMap;

	/*
	 * prefixMap is the map of prefixes to their count
	 * if their count is 1, their value is false
	 * if their count is more than 1, their value is true
	 *
	 * Example:
	 *
	 * if we have this synonym:
	 *      new york=>ny
	 *      new york city=>nyc
	 *
	 * The prefixMap's elements will be as following:
	 *      new => true (the count is 2)
	 *      new york => true (the count is 2)
	 *      new york city => false (the count is 1)
	 */
	map<std::string, bool> prefixMap;

	/*
	 * this a temporary buffer to keep the words that are waiting to get emit.
	 */
	vector<string> emitBuffer;

	/*
	 * It is a buffer for tokens to check if we have multi-word synonyms
	 */
	std::vector<std::string> tokenBuffer;

	/*
	 *  creates the map of stop words.
	 */
	void createMaps(const std::string &synonymFilePath);


	/*
	 *  Checks the synonym map and returns the number of keys which have the word as their substring happening at the begining
	 */
	int numberOfKeysHavingTokenAsPrefix(const std::string &);

	/*
	 * put the synonyms of existing tokens into emit token
	 */
	void pushSynonymsOfExistingTokensInEmitBuffer();

	/*
	 * returns the value of the string as the substring of a key
	 * returns NULL if there is no such a key
	 */
	const std::string getSynonymOf(const std::string &);

	/*
	 * returns true if the key is one of the keys of synonym map
	 * returns false otherwise
	 */
	bool isCurrentTokenExistInSynonymMap(const std::string &key);

	/*
	 * it emits the first member of the temporaryToken vedctor.
	 * sets the currentToken to the first member of this vector
	 * and removes the first member of the vefctor.
	 * calling this function should be followed by "return" in increment function.
	 *
	 */
	void emitCurrentToken();

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & synonymMap;
		ar & prefixMap;
		ar & emitBuffer;
		ar & tokenBuffer;
		ar & keepOriginFlag;
	}
};

}}
#endif /* __CORE_ANALYZER__SYNONYMFILTER_H_ANALYZER__ */
