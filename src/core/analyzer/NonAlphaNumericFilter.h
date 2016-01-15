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
/*
 * NonAlphaNumericFilter.h
 *
 *  Created on: Nov 13, 2013
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
	queue< std::pair<vector<CharType>, unsigned> > internalTokenBuffer;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* __CORE_ANALYZER_NONALPHANUMERICFILTER_H__ */
