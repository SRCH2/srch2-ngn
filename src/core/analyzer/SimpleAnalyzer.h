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
 * SimpleAnalyzer.h
 *
 *  Created on: 2013-5-18
 */
//This class will tokenize the words according to whitespace character .
#ifndef __CORE_ANALYZER_SIMPLEANALYZER_H__
#define __CORE_ANALYZER_SIMPLEANALYZER_H__

#include "AnalyzerInternal.h"
#include "TokenStream.h"

namespace srch2 {
namespace instantsearch {

/*
 *  SimpleAnalyzer use DELIMITER_TYPE to separate the string and use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokenized as "we" "went" "to" and "学校"
 */
class SimpleAnalyzer: public AnalyzerInternal {
public:

    SimpleAnalyzer(const StemmerContainer *stemmer,
                   const StopWordContainer *stopWords,
                   const ProtectedWordsContainer *protectedWords,
                   const SynonymContainer *synonyms,
                   const std::string &allowedRecordSpecialCharacters) :
        AnalyzerInternal(stemmer, stopWords, protectedWords, synonyms, allowedRecordSpecialCharacters)
        {
    		this->tokenStream = NULL;
            this->analyzerType = SIMPLE_ANALYZER;
        }

	SimpleAnalyzer(const SimpleAnalyzer &simpleAnalyzer) :
			AnalyzerInternal(simpleAnalyzer) {
		this->analyzerType = SIMPLE_ANALYZER;
	}
	TokenStream *createOperatorFlow();
	virtual ~SimpleAnalyzer();

    AnalyzerType getAnalyzerType() const;
};
}
}
#endif /* __CORE_ANALYZER__SIMPLEANALYZER_H__ */
