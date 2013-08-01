// $Id: Analyzer.cpp 3219 2013-03-25 23:36:34Z iman $

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

#include "AnalyzerInternal.h"
#include "StandardAnalyzer.h"
#include "SimpleAnalyzer.h"
#include <instantsearch/Analyzer.h>
#include <boost/algorithm/string.hpp>
#include <util/Assert.h>

namespace srch2 {
namespace instantsearch {
// TODO: remove create. The constructor is called directly
//Analyzer *Analyzer::create(const StemmerNormalizerFlagType &stemmerFlag,
//            const std::string &stemmerFilePath,
//            const std::string &stopWordFilePath,
//            const std::string &synonymFilePath,
//            const SynonymKeepOriginFlag &synonymKeepOriginFlag,
//            const std::string &delimiters,
//            const AnalyzerType &analyzerType) {
//
//
//}


Analyzer::Analyzer(const StemmerNormalizerFlagType &stemNormType,
        const std::string &stemmerFilePath,
        const std::string &stopWordFilePath,
        const std::string &synonymFilePath,
        const SynonymKeepOriginFlag &synonymKeepOriginFlag,
        const std::string &delimiters,
        const AnalyzerType &analyzerType){
    switch (analyzerType) {
        case SIMPLE_ANALYZER:
            this->analyzerInternal = new SimpleAnalyzer(stemNormType, stemmerFilePath, stopWordFilePath,
                            synonymFilePath, synonymKeepOriginFlag, delimiters);
            break;
        default:
            this->analyzerInternal = new StandardAnalyzer(stemNormType, stemmerFilePath, stopWordFilePath,
                            synonymFilePath, synonymKeepOriginFlag, delimiters);
            break;
        }
}

Analyzer::Analyzer(AnalyzerInternal *analyzerInternal) {
    this->analyzerInternal = analyzerInternal;
}

bool isEmpty(const string &inString)
{
    return inString.compare("") == 0;
}

// moved
void Analyzer::setRecordAllowedSpecialCharacters(
        const std::string &recordAllowedSpecialCharacters) {
    this->analyzerInternal->recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
    CharSet::setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
}

// moved
const std::string& Analyzer::getRecordAllowedSpecialCharacters() const {
    return this->analyzerInternal->recordAllowedSpecialCharacters;
}


// moved
void Analyzer::tokenizeQuery(const std::string &queryString,
        std::vector<std::string> &queryKeywords) const {
    queryKeywords.clear();
    this->analyzerInternal->loadData(queryString);
    string currentToken = "";
    while (this->analyzerInternal->tokenStream->processToken()) //process the token one by one
    {
        vector<CharType> charVector;
        charVector = this->analyzerInternal->tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, currentToken);
        queryKeywords.push_back(currentToken);
    }

    if (queryKeywords.size() == 1 && isEmpty(queryKeywords[0])){
        queryKeywords.clear();
    }
}

bool queryIsEmpty(string str) {
    return str.empty();
}
// TODO: Refactor the function and its arguments. Possibly move to wrapper
void Analyzer::tokenizeQueryWithFilter(const std::string &queryString,
        std::vector<std::string> &queryKeywords, const char &delimiterCharacter,
        const char &filterDelimiterCharacter, const char &fieldsAndCharacter,
        const char &fieldsOrCharacter,
        const std::map<std::string, unsigned> &searchableAttributesNameToId,
        std::vector<unsigned> &filters) const{

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

    vector <string> parts;
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
        vector <string> one_pair;
        boost::split(one_pair, parts[i], boost::is_any_of(" "));

        if (one_pair.size() > 2 || one_pair.size() == 0) {
            malformed = true;
            break;
        }
        // TODO: this or not this
        const string cleanString = this->analyzerInternal->cleanString(one_pair[0]);
        queryKeywords.push_back(cleanString);

        if (one_pair.size() == 1) {// have no filter information
            filters.push_back(0x7fffffff); // can appear in any field, the top bit is reserved for AND/OR relationship.
            continue;
        }

        vector <string> fields;

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

}
}
