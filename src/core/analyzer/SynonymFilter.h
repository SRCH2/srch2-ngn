// $Id: StopFilter.cpp 3074 2013-21-06 22:26:36Z iman $

/*
 * SynonymFilter.h
 *
 *  Created on: Jun 21, 2013
 *      Author: iman
 */

#ifndef __CORE_ANALYZER_SYNONYMFILTER_H__
#define __CORE_ANALYZER_SYNONYMFILTER_H__

#include <string>
#include <vector>
#include <map>

#include "TokenStream.h"
#include "TokenFilter.h"
#include "instantsearch/Analyzer.h"

/*
 * If we have folllwing synonym rules
 * s1: new york = ny
 * s2: new york city = nyc
 * s3: bill = william
 *
 * The flags will be set as following:
 * new: SYNONYM_PREFIX_ONLY, meaning it is a prefix of a left-hand-site (lhs) string (s1 and s2) but not a complete string.
 * new york: SYNONYM_PREFIX_AND_COMPLETE, meaning it is a prefix of an lhs string (s2) and also a complete lhs string (s1).
 * new york city: SYNONYM_COMPLETE_ONLY, meaning it is a complete lhs string (s2) but it's not a (proper) prefix of any lhs string.
 * bill: SYNONYM_COMPLETE_ONLY, meaning it is a complete lhs string (s3) but it's not a (proper) prefix of any lhs string.
 * orange: SYNONYM_NOT_PREFIX_NOT_COMPLETE, meaning it is not a complete lhs string nor a prefix of any lhs string.
 */
typedef enum{
	SYNONYM_PREFIX_ONLY,
	SYNONYM_COMPLETE_ONLY,
	SYNONYM_PREFIX_AND_COMPLETE,
	SYNONYM_NOT_PREFIX_NOT_COMPLETE
} SynonymTokenType;


using namespace std;

namespace srch2 {
namespace instantsearch {

class SynonymFilter: public TokenFilter {
public:
	/*
	 * Constructor of synonym filter.
	 * Sets sharedToken.
	 */
	SynonymFilter(TokenStream *tokenStream,
			const string &synonymFilterFilePath,
			const SynonymKeepOriginFlag &synonymKeepOriginFlag);

	/*
	 * IncrementToken() is a virtual function of class TokenOperator.
	 * Here we have to implement it. It goes on all tokens.
	 * */
	bool processToken();

	virtual ~SynonymFilter();

private:

	/*
	 * It is about keeping the original keyword or not
	 */
	srch2::instantsearch::SynonymKeepOriginFlag keepOriginFlag;

	const string synonymDelimiter;

	/*
	 * If we have folllwing synonym rules
	 * s1: new york = ny
	 * s2: new york city = nyc
	 * s3: bill = william
	 *
	 * The map elements will be as following:
	 *
	 * new => <SYNONYM_PREFIX_ONLY, "" >
	 * new york => <SYNONYM_PREFIX_AND_COMPLETE, 'ny'>
	 * new york city => <SYNONYM_COMPLETE_ONLY, 'nyc'>
	 * bill => <SYNONYM_COMPLETE_ONLY, 'william'>
	 * orange: Nothing will be in the map for it.
	 */
	map<string, pair<SynonymTokenType, string> > synonymMap;

	/*
	 * this a temporary buffer to keep the words that are waiting to get emit.
	 */
	vector<string> emitBuffer;

	/*
	 * It is a buffer for tokens to check if we have multi-word synonyms
	 */
	std::vector<string> tokenBuffer;

	/*
	 *  creates the map of stop words.
	 */
	void createMap(const string &synonymFilePath);

	/*
	 * put the synonyms of existing tokens into the buffer of to-be-emitted tokens
	 */
	void pushSynonymsOfExistingTokensInEmitBuffer();

	/*
	 * returns the row of the map which has input as its key
	 * returns NULL if there is no such a key
	 */
	pair<SynonymTokenType, std::string> getValuePairOf(const std::string &key);


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
		ar & emitBuffer;
		ar & tokenBuffer;
		ar & keepOriginFlag;
	}
};

}}
#endif /* __CORE_ANALYZER__SYNONYMFILTER_H_ANALYZER__ */
