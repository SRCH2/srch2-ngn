// $Id: AnalyzerInternal.cpp 3375 2013-05-24 06:54:28Z iman $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include <instantsearch/Record.h>
#include <instantsearch/Schema.h>
#include <util/Assert.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "AnalyzerInternal.h"
using std::pair;
using namespace std;

namespace srch2 {
namespace instantsearch {

/**
 * Helper function to tokenizeRecord
 * @param attribbute
 * @param position
 * @return an unsigned with bits set with the following logic:
 * Attribute -> First 8bits -> Attribute in which the token hit occurred
 * Position -> Last 24 bits -> Position within the attribute where the token hit occurred.
 */
unsigned setAttributePositionBitVector(unsigned attribute, unsigned position) {
    ///assert that attribute is less than maximum allowed attributes
    ASSERT(attribute < 0xff);

    ///assert that position is less than maximum allowed document size
    ASSERT(position < 0xffffff);

    return ((attribute + 1) << 24) + (position & 0xffffff);

//    return 1 << attribute;
}

bool isEmpty(const string &inString) {
    return inString.compare("") == 0;
}

/*
 * Important:
 *   Since this AnalyzerInternal class can be inherited by other superclasses, it's a common practice to initialize these
 *   parameters in this superclass, rather than doing the initialization in the subclasses. In fact, if we let the subclasses
 *   do the initialization, for some reason our engine doesn't work on Android.
 */

AnalyzerInternal::AnalyzerInternal(const AnalyzerInternal &analyzerInternal) {
    this->recordAllowedSpecialCharacters =
            analyzerInternal.recordAllowedSpecialCharacters;
    prepareRegexExpression();
    tokenStreamContainer.reset(new TokenStreamContainer);
    this->stemmerFlag = analyzerInternal.stemmerFlag;
    this->stemmerFilePath = analyzerInternal.stemmerFilePath;
    this->stopWordFilePath = analyzerInternal.stopWordFilePath;
    this->synonymFilePath = analyzerInternal.synonymFilePath;
    this->synonymKeepOriginFlag = analyzerInternal.synonymKeepOriginFlag;
}

AnalyzerInternal::AnalyzerInternal(const StemmerNormalizerFlagType &stemmerFlag,
        const string &recordAllowedSpecialCharacters,
        const string &stemmerFilePath, const string &stopWordFilePath,
        const string &synonymFilePath,
        const SynonymKeepOriginFlag &synonymKeepOriginFlag) {

    this->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
    CharSet::setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
    prepareRegexExpression();
    tokenStreamContainer.reset(new TokenStreamContainer);
    this->stemmerFlag = stemmerFlag;
    this->stemmerFilePath = stemmerFilePath;
    this->stopWordFilePath = stopWordFilePath;
    this->synonymFilePath = synonymFilePath;
    this->synonymKeepOriginFlag = synonymKeepOriginFlag;

}

void AnalyzerInternal::loadData(const string &s) const {
    std::vector<CharType> charVector;
    utf8StringToCharTypeVector(s, charVector); //clean the string and convert the string to CharTypeVector
    this->tokenStreamContainer->currentToken.clear();
    this->tokenStreamContainer->completeCharVector = charVector;
    this->tokenStreamContainer->offset = 0;
}

/**
 * Function to tokenize a given record.
 * @param[in] record
 * @param[in, out] tokenAttributeHitsMap
 *
 * Pseudocode:
 * 1. map<string, TokenAttributeHits > tokenAttributeHitsMap of a record is a map from unique tokens to their corresponding attributeHits information(Refer Struct TokenAttributeHits )
 * 2. First, we iterate over the record attributes. So, for each recordAttribute
 *      1. Boost::split is used to get the tokens from attributeValue.
 *      2. We iterate over these tokens and fill the map
 */
void AnalyzerInternal::tokenizeRecord(const Record *record,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) const {
    tokenAttributeHitsMap.clear();
    const Schema *schema = record->getSchema();
    // token string to vector<CharType>
    vector<string> tokens;
    vector<string>::iterator tokenIterator;
    unsigned positionIterator;
    unsigned size;
    for (unsigned attributeIterator = 0;
            attributeIterator < schema->getNumberOfSearchableAttributes();
            attributeIterator++) {
        string *attributeValue = record->getSearchableAttributeValue(
                attributeIterator);
        if (attributeValue != NULL) {
            tokens.clear();
            loadData(*attributeValue);
            string currentToken = "";
            while (tokenStream->processToken()) //process the token one by one
            {
                vector<CharType> charVector;
                charVector = tokenStream->getProcessedToken();
                charTypeVectorToUtf8String(charVector, currentToken);
                tokens.push_back(currentToken);
                //cout<<currentToken<<endl;
            }
            size = tokens.size();

            positionIterator = 1;
            for (tokenIterator = tokens.begin(); tokenIterator != tokens.end();
                    ++tokenIterator, ++positionIterator) {

                ///TODO OPT: Remove this comparison and make sure, split returns no empty strings.
                if (tokenIterator->size()) {
                    //Convert to lowercase
                    std::transform(tokenIterator->begin(), tokenIterator->end(),
                            tokenIterator->begin(), ::tolower);

                    tokenAttributeHitsMap[*tokenIterator].attributeList.push_back(
                            setAttributePositionBitVector(attributeIterator,
                                    positionIterator));
                }
            }

        }
    }
}

//token utf-8 string to vector<vector<CharType> >
void AnalyzerInternal::tokenizeQuery(const string &queryString,
        vector<string> &queryKeywords) const {
    queryKeywords.clear();
    loadData(queryString);
    string currentToken = "";
    while (tokenStream->processToken()) //process the token one by one
    {
        vector<CharType> charVector;
        charVector = this->tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, currentToken);
        queryKeywords.push_back(currentToken);
        //cout<<currentToken<<endl;
    }

    if (queryKeywords.size() == 1 && isEmpty(queryKeywords[0]))
        queryKeywords.clear();
}

bool queryIsEmpty(string str) {
    return str.empty();
}

void AnalyzerInternal::tokenizeQueryWithFilter(const string &queryString,
        vector<string> &queryKeywords, const char &delimiterCharacter,
        const char &filterDelimiterCharacter, const char &fieldsAndCharacter,
        const char &fieldsOrCharacter,
        const std::map<string, unsigned> &searchableAttributesNameToId,
        vector<unsigned> &filters) const {
    stringstream charToString;
    string delimiter;
    string filterDelimiter;
    string fieldDelimiter;
    charToString << delimiterCharacter;
    charToString >> delimiter;
    charToString.clear();
    charToString << filterDelimiterCharacter;
    charToString >> filterDelimiter;
    charToString.clear();
    charToString << fieldsAndCharacter;
    charToString << fieldsOrCharacter;
    charToString >> fieldDelimiter;

    string query = queryString;
    queryKeywords.clear();
    filters.clear();
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    vector<string> parts;
    replace_if(query.begin(), query.end(), boost::is_any_of(delimiter),
            DEFAULT_DELIMITER);

    boost::split(parts, query, boost::is_any_of(" "));
    std::vector<string>::iterator iter = std::remove_if(parts.begin(),
            parts.end(), queryIsEmpty);
    parts.erase(iter, parts.end());
    // print the queries
    //std::cout<<"parts number:" << parts.size()<<std::endl;

    bool malformed = false;
    for (unsigned i = 0; i < parts.size(); i++) {
        replace_if(parts[i].begin(), parts[i].end(),
                boost::is_any_of(filterDelimiter), DEFAULT_DELIMITER);
        vector<string> one_pair;
        boost::split(one_pair, parts[i], boost::is_any_of(" "));

        if (one_pair.size() > 2 || one_pair.size() == 0) {
            malformed = true;
            break;
        }

        const string cleanString = this->cleanString(one_pair[0]);
        queryKeywords.push_back(cleanString);

        if (one_pair.size() == 1) {        // have no filter information
            filters.push_back(0x7fffffff); // can appear in any field, the top bit is reserved for AND/OR relationship.
            continue;
        }

        vector<string> fields;

        bool AND = false;
        bool OR = false;
        if (one_pair[1].find(fieldsAndCharacter) != string::npos)
            AND = true;
        if (one_pair[1].find(fieldsOrCharacter) != string::npos)
            OR = true;
        if (AND && OR) {
            malformed = true;
            break;
        }

        boost::split(fields, one_pair[1], boost::is_any_of(fieldDelimiter));
        unsigned filter = 0;
        for (unsigned j = 0; j < fields.size(); j++) {
            map<string, unsigned>::const_iterator iter =
                    searchableAttributesNameToId.find(fields[j]);
            if (iter == searchableAttributesNameToId.end()) {
                malformed = true;
                break;
            }

            unsigned bit = 1;
            ASSERT(iter->second < 31); // the left most bit is reserved for indicating the AND/OR relation between fields
            bit <<= iter->second;
            filter |= bit;
        }
        if (AND)
            filter |= 0x80000000;

        if (malformed)
            break;
        else
            filters.push_back(filter);
    }

    if (malformed || (queryKeywords.size() == 1 && isEmpty(queryKeywords[0])))
        queryKeywords.clear();
}

const AnalyzerType& AnalyzerInternal::getAnalyzerType() const{
    return this->analyzerType;
}

const StemmerNormalizerFlagType& AnalyzerInternal::getStemmerFlag() const{
    return this->stemmerFlag;
}

const string& AnalyzerInternal::getRecordAllowedSpecialCharacters() const{
    return this->recordAllowedSpecialCharacters;
}

const string& AnalyzerInternal::getStopWordFilePath() const{
    return stopWordFilePath;
}

const string& AnalyzerInternal::getSynonymFilePath() const{
    return this->synonymFilePath;
}

const string& AnalyzerInternal::getStemmerFilePath() const{
    return this->stemmerFilePath;
}

const SynonymKeepOriginFlag& AnalyzerInternal::getSynonymKeepOriginFlag() const{
    return synonymKeepOriginFlag;
}


}
}
