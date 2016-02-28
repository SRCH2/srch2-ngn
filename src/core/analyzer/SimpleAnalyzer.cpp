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
 * SimpleAnalyzer.cpp
 *
 *  Created on: 2013-5-18
 */

#include "SimpleAnalyzer.h"
#include "WhiteSpaceTokenizer.h"
#include "LowerCaseFilter.h"
#include "StemmerFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"
#include "util/Logger.h"
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

// create operator flow and link share pointer to the data
TokenStream * SimpleAnalyzer::createOperatorFlow() {
	TokenStream *tokenStream = new WhiteSpaceTokenizer();

	tokenStream = new LowerCaseFilter(tokenStream);
        if (stopWords != NULL) {
            tokenStream = new StopFilter(tokenStream, stopWords);
        }
        if (synonyms != NULL) {
            tokenStream = new SynonymFilter(tokenStream, synonyms);
        }
        if (stemmer != NULL) {
            tokenStream = new StemmerFilter(tokenStream, stemmer);
        }

	return tokenStream;
}
SimpleAnalyzer::~SimpleAnalyzer() {
	delete this->tokenStream;
}

AnalyzerType SimpleAnalyzer::getAnalyzerType() const
{
    return SIMPLE_ANALYZER;
}

}
}

