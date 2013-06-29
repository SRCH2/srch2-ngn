/*
 * AnalyzerInternal.h
 *
 *  Created on: 2013-5-17
 *
 */

#ifndef __ANALYZER_INTERNAL_H__
#define __ANALYZER_INTERNAL_H__

#include <vector>
#include <map>
#include <fstream>
#include <instantsearch/Analyzer.h>
#include <boost/regex.hpp>
#include "TokenOperator.h"

using std::vector;
using std::map;
using std::string;
using std::stringstream;
namespace srch2
{
namespace instantsearch
{

class Record;

struct TokenAttributeHits {
    /** Each entry has position information as follows:
     *  Attribute -> First 8bits -> Attribute in which the token hit occurred
     *  Hits -> Last 24 bits -> Position within the attribute where the token hit occurred.
     *  The positions start from 1, this is because the positions in PositionIndex are ZERO terminated.
     *
     *  The maximum number of allowed Attributes is checked by the following assert
     *  ASSERT( attribute <  0xff);
     *
     *  i.e. 255
     *
     *  The maximum number of the positionHit is checked by the following assert
     *  ASSERT( position <  0xffffff);
     *
     * i.e. 4 294 967 295
     *
     */
    vector<unsigned> attributeList;
};

class AnalyzerInternal: public Analyzer{
public:
	AnalyzerInternal(const StemmerNormalizerFlagType &stemmerFlag, const std::string &recordAllowedSpecialCharacters);
	AnalyzerInternal(const AnalyzerInternal &analyzerInternal);
	// assign the shared ptr to given string, clean currentToken and reset offset
	void loadData(const std::string &s) const;

	virtual TokenOperator * createOperatorFlow() = 0;
	virtual ~AnalyzerInternal() {}

	/*
	 *  Analyzer allows a set of special characters in queries. These two functions are setter/getter
	 *  for setting/getting the special characters.
	 */
	void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters)
	{
		this->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
		CharSet::setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
	}
	const std::string& getRecordAllowedSpecialCharacters() const
	{
		return this->recordAllowedSpecialCharacters;
	}

	void prepareRegexExpression()
	{
		//allow all characters
		string regexString = "[^A-Za-z0-9 " + CharSet::getRecordAllowedSpecialCharacters() +"\x80-\xFF" + "]";
		try{
			disallowedCharactersRegex = boost::regex(regexString);
		}
		catch(boost::regex_error& e) {

			std::cerr << regexString << " is not a valid regular expression. Using default: [^A-Za-z0-9 ]" << std::endl;
			disallowedCharactersRegex = boost::regex("[^A-Za-z0-9 ]");
		}

		multipleSpaceRegex = boost::regex(" +");
		headTailSpaceRegex = boost::regex("^[ \t]+|[ \t]+$");
	}

	const string cleanString(const std::string &inputString) const
	{
		// example: " ab$(cd " -> " ab  cd "
		const std::string string1 = boost::regex_replace(inputString, disallowedCharactersRegex, DEFAULT_DELIMITER_STRING);

		// example: " ab  cd " -> " ab cd "
		const std::string string2 = boost::regex_replace(string1, multipleSpaceRegex, DEFAULT_DELIMITER_STRING);

		// example: " ab cd " -> "ab cd"
		const std::string string3 = boost::regex_replace(string2, headTailSpaceRegex, "");

		return string3;
	}

	static void load(AnalyzerInternal &analyzer, boost::archive::binary_iarchive &ia)
	{
		ia >> analyzer;
	};
	const AnalyzerType& getAnalyzerType() const
	{
		return analyzerType;
	}

	static void save(const AnalyzerInternal &analyzer, boost::archive::binary_oarchive &oa)
	{
		oa << analyzer;
	};

	/**
	 * Function to tokenize a given record.
	 * @param[in] record
	 * @param[in, out] tokenAttributeHitsMap
	 */
	void tokenizeRecord(const Record *record, map<string, TokenAttributeHits > &tokenAttributeHitsMap) const;

	/**
	 * Function to tokenize a given query.
	 * Remove duplicates like in query, "nose bleed nose" -> "nose bleed"
	 * @param[in] queryString
	 * @param[in, out] queryKeywords
	 * @param[in] delimiterCharater
	 */
	void tokenizeQuery(const string &queryString, vector<string> &queryKeywords) const;

	void tokenizeQueryWithFilter(const string &queryString, vector<string> &queryKeywords, const char &delimiterCharacter,
								 const char &filterDelimiterCharacter, const char &fieldsAndCharacter, const char &fieldsOrCharacter,
								 const std::map<std::string, unsigned> &searchableAttributesNameToId, vector<unsigned> &filter) const;



protected:
	boost::shared_ptr<SharedToken > sharedToken;

	TokenOperator* tokenOperator;
	string recordAllowedSpecialCharacters;
	AnalyzerType analyzerType;
	StemmerNormalizerFlagType stemmerType; // know if we are stemming or not

    boost::regex disallowedCharactersRegex;
    boost::regex multipleSpaceRegex;
    boost::regex headTailSpaceRegex;

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & recordAllowedSpecialCharacters;
		ar & analyzerType;
	}

};


}}
#endif /* __ANALYZER_INTERNAL_H__ */
