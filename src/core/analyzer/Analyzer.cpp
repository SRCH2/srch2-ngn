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
#include "ChineseAnalyzer.h"
#include <instantsearch/Analyzer.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <util/Assert.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>


using std::stringstream;

namespace srch2 {
namespace instantsearch {

Analyzer::Analyzer(const StemmerNormalizerFlagType &stemmerFlag,
        const std::string &stemmerFilePath, const std::string &stopWordFilePath,
                   const std::string &protectedWordFilePath,
        const std::string &synonymFilePath,
        const SynonymKeepOriginFlag &synonymKeepOriginFlag,
        const std::string &delimiters, 
        const AnalyzerType &analyzerType, 
        const std::string &chineseDictFilePath
        ) {
    switch (analyzerType) {
    case SIMPLE_ANALYZER:
        this->analyzerInternal = new SimpleAnalyzer(stemmerFlag,
                stemmerFilePath, stopWordFilePath, synonymFilePath,
                synonymKeepOriginFlag, delimiters);
        break;
    case CHINESE_ANALYZER:
        this->analyzerInternal = new ChineseAnalyzer(chineseDictFilePath,
                delimiters, stopWordFilePath, synonymFilePath,
                synonymKeepOriginFlag);
        break;
    case STANDARD_ANALYZER:
    default:
        this->analyzerInternal = new StandardAnalyzer(stemmerFlag,
                stemmerFilePath, stopWordFilePath, protectedWordFilePath, synonymFilePath,
                synonymKeepOriginFlag, delimiters);
        break;
    }
    analyzerInternal->setTokenStream( analyzerInternal->createOperatorFlow());
}

Analyzer::Analyzer(Analyzer& analyzerObject) {
    switch (analyzerObject.analyzerInternal->getAnalyzerType()) {
    case SIMPLE_ANALYZER:
        this->analyzerInternal = new SimpleAnalyzer(
                static_cast<const SimpleAnalyzer&>(*(analyzerObject.analyzerInternal)));
        break;
    case CHINESE_ANALYZER:
        this->analyzerInternal = new ChineseAnalyzer(
                static_cast<const ChineseAnalyzer&>(*(analyzerObject.analyzerInternal)));
        break;
    case STANDARD_ANALYZER:
    default:
        this->analyzerInternal = new StandardAnalyzer(
                static_cast<const StandardAnalyzer&>(*(analyzerObject.analyzerInternal)));
        break;
    }
    analyzerInternal->setTokenStream( analyzerInternal->createOperatorFlow());
}

void Analyzer::setRecordAllowedSpecialCharacters(
        const std::string &recordAllowedSpecialCharacters) {
    this->analyzerInternal->setRecordAllowedSpecialCharacters(recordAllowedSpecialCharacters);
}

const std::string& Analyzer::getRecordAllowedSpecialCharacters() const {
    return this->analyzerInternal->getRecordAllowedSpecialCharacters();
}

std::string Analyzer::applyFilters(std::string input, bool isPrefix = false){
    return this->analyzerInternal->applyFilters(input, isPrefix);
}

void Analyzer::clearFilterStates(){
    this->analyzerInternal->clearFilterStates();
}

void Analyzer::tokenizeQuery(const std::string &queryString,
        std::vector<PositionalTerm> &queryKeywords) const {
    this->analyzerInternal->tokenizeQuery(queryString, queryKeywords);
}

void Analyzer::tokenizeRecord(const Record *record,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) const {
    this->analyzerInternal->tokenizeRecord(record,tokenAttributeHitsMap);
}

const AnalyzerType& Analyzer::getAnalyzerType() const {
    return this->analyzerInternal->getAnalyzerType();
}

// TODO: Refactor the function and its arguments. Possibly move to wrapper
void Analyzer::tokenizeQueryWithFilter(const std::string &queryString,
        std::vector<PositionalTerm> &queryKeywords, const char &delimiterCharacter,
        const char &filterDelimiterCharacter, const char &fieldsAndCharacter,
        const char &fieldsOrCharacter,
        const std::map<std::string, unsigned> &searchableAttributesNameToId,
        std::vector<unsigned> &filters) const {
    this->analyzerInternal->tokenizeQueryWithFilter(queryString, queryKeywords,
            delimiterCharacter, filterDelimiterCharacter, fieldsAndCharacter,
            fieldsOrCharacter, searchableAttributesNameToId, filters);
}

void Analyzer::load(boost::archive::binary_iarchive &ia){
    this->analyzerInternal->load(ia);
}

void Analyzer::save(boost::archive::binary_oarchive &oa) {
    this->analyzerInternal->save(oa);
}

Analyzer::~Analyzer() {
    delete this->analyzerInternal;
}


}
}
