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
 * AnalyzerInternal.h
 *
 *  Created on: 2013-5-17
 *
 */

#ifndef __CORE_ANALYZER_ANALYZER_INTERNAL_H__
#define __CORE_ANALYZER_ANALYZER_INTERNAL_H__

#include <vector>
#include <map>
#include <fstream>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Constants.h>
#include <boost/regex.hpp>
#include <string>
#include "TokenStream.h"
#include "util/Logger.h"

using std::vector;
using std::map;
using std::string;
using std::stringstream;
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

class Record;

class AnalyzerInternal {
public:

	AnalyzerInternal(const AnalyzerInternal &analyzerInternal);

	AnalyzerInternal(const StemmerContainer *stemmer,
                         const StopWordContainer *stopWords,
                         const ProtectedWordsContainer *protectedWords,
                         const SynonymContainer *synonyms,
                         const std::string &recordAllowedSpecialCharacters
                         );

	void clearFilterStates();

    void setTokenStream(TokenStream* stream){
        this->tokenStream = stream;
        tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
        prepareRegexExpression();
    }

	virtual TokenStream * createOperatorFlow() = 0;

	virtual ~AnalyzerInternal() {};

	 /*
     *  Analyzer allows a set of special characters in queries. These two functions are setter/getter
     *  for setting/getting the special characters.
     */
    void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters)
    {
        this->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
        if (tokenStream != NULL) {
            tokenStream->characterSet.setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
            prepareRegexExpression();
        }
    }

	void prepareRegexExpression() {
		//allow all characters
		string regexString = "[^A-Za-z0-9 "
				+ tokenStream->characterSet.getRecordAllowedSpecialCharacters() + "\x80-\xFF"
				+ "]";
		try {
			disallowedCharactersRegex = boost::regex(regexString);
		} catch (boost::regex_error& e) {

            Logger::error("%s is not a valid regular expression. Using default: [^A-Za-z0-9 ]", regexString.c_str());
			disallowedCharactersRegex = boost::regex("[^A-Za-z0-9 ]");
		}

		multipleSpaceRegex = boost::regex(" +");
		headTailSpaceRegex = boost::regex("^[ \t]+|[ \t]+$");
	}

	const string cleanString(const std::string &inputString) const {
		// example: " ab$(cd " -> " ab  cd "
		const std::string string1 = boost::regex_replace(inputString,
				disallowedCharactersRegex, DEFAULT_DELIMITER_STRING);
		// example: " ab  cd " -> " ab cd "
		const std::string string2 = boost::regex_replace(string1,
				multipleSpaceRegex, DEFAULT_DELIMITER_STRING);

		// example: " ab cd " -> "ab cd"
		const std::string string3 = boost::regex_replace(string2,headTailSpaceRegex, "");
		return string3;
	}

	void load(boost::archive::binary_iarchive &ia);

	void save(boost::archive::binary_oarchive &oa) {
		oa << *this;
	};

	/**
	 * Function to tokenize a given record.
	 * @param[in] record
	 * @param[in, out] tokenAttributeHitsMap
	 */
	void tokenizeRecord(const Record *record,
			map<string, TokenAttributeHits> &tokenAttributeHitsMap) const;

	 /**
     * Function to tokenize a given query.
     * Remove duplicates like in query, "nose bleed nose" -> "nose bleed"
     * @param[in] queryString
     * @param[in, out] queryKeywords
     * @param[in] delimiterCharater
     */
    void tokenizeQuery(const string &queryString,
                    vector<AnalyzedTermInfo> &queryKeywords, bool isPrefix = false) const;


    // getter for the protected fields
    virtual AnalyzerType getAnalyzerType() const = 0;
    const string& getRecordAllowedSpecialCharacters() const;

    TokenStream* getTokenStream();

protected:
	TokenStream* tokenStream;
	AnalyzerType analyzerType;

        // Data source specific filter data
        // (will need to change from references to pointers if we ever want to re-use analyzers with different data sources)
        std::string recordAllowedSpecialCharacters;
        const StemmerContainer *stemmer;
        const StopWordContainer *stopWords;
        const SynonymContainer *synonyms;
        const ProtectedWordsContainer *protectedWords;

	boost::regex disallowedCharactersRegex;
	boost::regex multipleSpaceRegex;
	boost::regex headTailSpaceRegex;

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & recordAllowedSpecialCharacters;
		ar & analyzerType;
	}

};

}
}
#endif /* __CORE_ANALYZER__ANALYZER_INTERNAL_H__ */
