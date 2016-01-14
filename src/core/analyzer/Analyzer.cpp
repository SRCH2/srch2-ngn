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


#include "AnalyzerInternal.h"
#include "AnalyzerContainers.h"
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

Analyzer::Analyzer(const StemmerContainer *stemmer,
                   const StopWordContainer *stopWords,
                   const ProtectedWordsContainer *protectedWords,
                   const SynonymContainer *synonyms,
                   const std::string &allowedSpecialCharacters,
                   const AnalyzerType &analyzerType,
                   const ChineseDictionaryContainer* chineseDictionaryContainer
                   )
{
    switch (analyzerType) {
    case SIMPLE_ANALYZER:
        this->analyzerInternal = new SimpleAnalyzer(stemmer, stopWords, protectedWords, synonyms,
                                                    allowedSpecialCharacters);
        break;
    case CHINESE_ANALYZER:
        this->analyzerInternal = new ChineseAnalyzer(chineseDictionaryContainer,
                                                     stopWords, protectedWords, synonyms,
                                                     allowedSpecialCharacters);
        break;
    case STANDARD_ANALYZER:
    default:
        this->analyzerInternal = new StandardAnalyzer(stemmer, stopWords, protectedWords, synonyms,
                                                      allowedSpecialCharacters);
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

void Analyzer::clearFilterStates(){
    this->analyzerInternal->clearFilterStates();
}

void Analyzer::tokenizeQuery(const std::string &queryString,
        std::vector<AnalyzedTermInfo> &queryKeywords,  bool isPrefix) const {
    this->analyzerInternal->tokenizeQuery(queryString, queryKeywords, isPrefix);
}

void Analyzer::tokenizeRecord(const Record *record,
        map<string, TokenAttributeHits> &tokenAttributeHitsMap) const {
    this->analyzerInternal->tokenizeRecord(record,tokenAttributeHitsMap);
}

AnalyzerType Analyzer::getAnalyzerType() const
{
    return this->analyzerInternal->getAnalyzerType();
}


void Analyzer::load(boost::archive::binary_iarchive &ia){
    this->analyzerInternal->load(ia);
}

void Analyzer::fillInCharacters(const char *data) {
	this->analyzerInternal->getTokenStream()->fillInCharacters(data);
}
bool Analyzer::processToken() {
	return this->analyzerInternal->getTokenStream()->processToken();
}
std::vector<CharType> & Analyzer::getProcessedToken() {
	return this->analyzerInternal->getTokenStream()->getProcessedToken();
}
unsigned Analyzer::getProcessedTokenCharOffset() {
	return this->analyzerInternal->getTokenStream()->getProcessedTokenCharOffset();
}
unsigned Analyzer::getProcessedTokenPosition() {
	return this->analyzerInternal->getTokenStream()->getProcessedTokenPosition();
}

unsigned Analyzer::getProcessedTokenLen() {
	return this->analyzerInternal->getTokenStream()->getProcessedTokenLen();
}

AnalyzedTokenType Analyzer::getProcessedTokenType() {
	return this->analyzerInternal->getTokenStream()->getProcessedTokentype();
}

void Analyzer::save(boost::archive::binary_oarchive &oa) {
    this->analyzerInternal->save(oa);
}

Analyzer::~Analyzer() {
    delete this->analyzerInternal;
}


}
}
