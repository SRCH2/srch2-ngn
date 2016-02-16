/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
#include "analyzer/AnalyzerContainers.h"

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
class SynonymContainer;
class SynonymFilter: public TokenFilter {
public:
	/*
	 * Constructor of synonym filter.
	 * Sets sharedToken.
	 */
	SynonymFilter(TokenStream *tokenStream,
                      const SynonymContainer *synonymContainer);

	/*
	 * IncrementToken() is a virtual function of class TokenOperator.
	 * Here we have to implement it. It goes on all tokens.
	 * */
	bool processToken();

	virtual ~SynonymFilter();

	void clearState();

private:

	/*
	 * It is about keeping the original keyword or not
	 */
	srch2::instantsearch::SynonymKeepOriginFlag keepOriginFlag;

	/*
	 * this a temporary buffer to keep the words that are waiting to get emit.
	 */
	vector<AnalyzedTermInfo> emitBuffer;

	/*
	 * It is a buffer for tokens to check if we have multi-word synonyms
	 */
	std::vector<AnalyzedTermInfo> tokenBuffer;

	/*
	 * put the synonyms of existing tokens into the buffer of to-be-emitted tokens
	 */
	void pushSynonymsOfExistingTokensInEmitBuffer();

	/*
	 * returns the row of the map which has input as its key
	 * returns NULL if there is no such a key
	 */
	bool getSynonymValuesOf(const std::string &key,  SynonymVector& synonyms) const;


	/*
	 * it emits the first member of the temporaryToken vedctor.
	 * sets the currentToken to the first member of this vector
	 * and removes the first member of the vefctor.
	 * calling this function should be followed by "return" in increment function.
	 *
	 */
	void emitCurrentToken();

	bool isPrefixToken(const string& str) const;

	const SynonymContainer *synonymContainer;
};

}}
#endif /* __CORE_ANALYZER__SYNONYMFILTER_H_ANALYZER__ */
