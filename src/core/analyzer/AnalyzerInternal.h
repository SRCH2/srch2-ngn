/*
 * AnalyzerInternal.h
 *
 *  Created on: 2013-5-17
 *
 */

#ifndef __CORE_ANALYZER__ANALYZER_INTERNAL_H__
#define __CORE_ANALYZER__ANALYZER_INTERNAL_H__

#include <vector>
#include <map>
#include <fstream>
#include <instantsearch/Analyzer.h>
#include <boost/regex.hpp>
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

	AnalyzerInternal(const StemmerNormalizerFlagType &stemmerFlag,
			const std::string &recordAllowedSpecialCharacters,
			const std::string &stemmerFilePath = "",
			const std::string &stopWordFilePath = "",
			const std::string &synonymFilePath = "",
			const SynonymKeepOriginFlag &synonymKeepOriginFlag = SYNONYM_KEEP_ORIGIN);

	void loadData(const std::string &s) const;


	virtual TokenStream * createOperatorFlow() = 0;

	virtual ~AnalyzerInternal() {};

	 /*
     *  Analyzer allows a set of special characters in queries. These two functions are setter/getter
     *  for setting/getting the special characters.
     */
    void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters) {
            this->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
            CharSet::setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
    }


    const std::string& getRecordAllowedSpecialCharacters() const {
            return this->recordAllowedSpecialCharacters;
    }


	void prepareRegexExpression() {
		//allow all characters
		string regexString = "[^A-Za-z0-9 "
				+ CharSet::getRecordAllowedSpecialCharacters() + "\x80-\xFF"
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

	void load(boost::archive::binary_iarchive &ia) {
		ia >> *this;
		Logger::debug("#### AnalyzerInternal Variables:   \n");
		Logger::debug("Stemmer Flag:                  %s\n", this->stemmerFlag);
		Logger::debug("Stemmer File Path :            %s\n", this->stemmerFilePath.c_str());
		Logger::debug("Stop Word File Path:           %s\n", this->stopWordFilePath.c_str());
		Logger::debug("Synonym File Path is:          %s\n", this->synonymFilePath.c_str());
		Logger::debug("Synonym Keep Origin Flag is:   %s\n", this->synonymKeepOriginFlag);
		Logger::debug("Analyzer Type:                 %s\n\n\n", this->analyzerType);
	    return;
	};
	const AnalyzerType& getAnalyzerType() const {
		return analyzerType;
	}

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
                    vector<string> &queryKeywords) const;

    void tokenizeQueryWithFilter(const string &queryString,
                    vector<string> &queryKeywords, const char &delimiterCharacter,
                    const char &filterDelimiterCharacter,
                    const char &fieldsAndCharacter, const char &fieldsOrCharacter,
                    const std::map<std::string, unsigned> &searchableAttributesNameToId,
                    vector<unsigned> &filter) const;


protected:
	boost::shared_ptr<TokenStreamContainer> tokenStreamContainer;

	TokenStream* tokenStream;
	AnalyzerType analyzerType;
	StemmerNormalizerFlagType stemmerFlag; // This flag shows that we want to stem or not.
	string recordAllowedSpecialCharacters;
	string stopWordFilePath;
	string synonymFilePath;
	string stemmerFilePath;
	SynonymKeepOriginFlag synonymKeepOriginFlag;

	boost::regex disallowedCharactersRegex;
	boost::regex multipleSpaceRegex;
	boost::regex headTailSpaceRegex;

	friend class boost::serialization::access;
	friend class Analyzer;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & recordAllowedSpecialCharacters;
		ar & analyzerType;
		ar & stopWordFilePath;
		ar & synonymFilePath;
		ar & synonymKeepOriginFlag;
		ar & stemmerFilePath;
		ar & stemmerFlag;
	}

};

}
}
#endif /* __CORE_ANALYZER__ANALYZER_INTERNAL_H__ */
