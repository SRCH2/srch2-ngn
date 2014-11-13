// $Id: QueryResultsInternal.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */

#include "QueryResultsInternal.h"
#include "operation/TermVirtualList.h"
//#include "index/Trie.h"
#include "operation/QueryEvaluatorInternal.h"
#include "util/Logger.h"
#include <instantsearch/Query.h>
//#include <instantsearch/Term.h>
#include <sstream>

using std::vector;
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {
class Term;
class Query;
class IndexSearcherInternal;
class TermVirtualList;




/////////////////////////////////////////// QueryResultsInternal Implementation ///////////////////////////////////////////////////////////////

QueryResultsInternal::QueryResultsInternal() {
    Logger::debug("Query Results internal created.");
    this->virtualListVector = NULL;
    this->stat = NULL;
    this->resultsApproximated = false;
    this->estimatedNumberOfResults = -1;
}

void QueryResultsInternal::init(QueryResultFactory * resultsFactory,
        const QueryEvaluatorInternal *queryEvaluatorInternal, Query *query) {
    Logger::debug("Query Results internal initialized.");
    this->resultsFactory = resultsFactory;
    this->query = query;
    this->virtualListVector = new vector<TermVirtualList*>;
    this->queryEvaluatorInternal = queryEvaluatorInternal;
    this->stat = new Stat();
    this->resultsApproximated = false;
    this->estimatedNumberOfResults = -1;
}

QueryResultsInternal::QueryResultsInternal(QueryResultFactory * resultsFactory,
		const QueryEvaluatorInternal *queryEvaluatorInternal, Query *query) {
    this->resultsFactory = resultsFactory;
    this->query = query;
    this->virtualListVector = new vector<TermVirtualList*>;
    this->queryEvaluatorInternal = queryEvaluatorInternal;
    this->stat = new Stat();
    this->resultsApproximated = false;
    this->estimatedNumberOfResults = -1;
}

QueryResultsInternal::QueryResultsInternal(const QueryResultsInternal & copy){
	this->resultsFactory = copy.resultsFactory;
	this->query = copy.query;
	// TODO : since this member is not used anymore. We must delete it eventually.
	this->virtualListVector = new vector<TermVirtualList*>;
    this->queryEvaluatorInternal = copy.queryEvaluatorInternal;
    this->stat = new Stat(*(copy.stat));
    this->resultsApproximated = copy.resultsApproximated;
    this->estimatedNumberOfResults = copy.estimatedNumberOfResults;
    this->sortedFinalResults = copy.sortedFinalResults;
    this->facetResults = copy.facetResults;
    this->prefixToCompleteStore = copy.prefixToCompleteStore;
}
QueryResultsInternal & QueryResultsInternal::operator=(const QueryResultsInternal & rhs){
	if(this != &rhs){
		this->resultsFactory = rhs.resultsFactory;
		this->query = rhs.query;
		// TODO : since this member is not used anymore. We must delete it eventually.
		this->virtualListVector = new vector<TermVirtualList*>;
	    this->queryEvaluatorInternal = rhs.queryEvaluatorInternal;
	    this->stat = new Stat(*(rhs.stat));
	    this->resultsApproximated = rhs.resultsApproximated;
	    this->estimatedNumberOfResults = rhs.estimatedNumberOfResults;
	    this->sortedFinalResults = rhs.sortedFinalResults;
	    this->facetResults = rhs.facetResults;
	    this->prefixToCompleteStore = rhs.prefixToCompleteStore;
	}
	return *this;
}

// DEBUG function. Used in CacheIntegration_Test
bool QueryResultsInternal::checkCacheHit(
        QueryEvaluatorInternal *queryEvaluatorInternal, Query *query) {
    this->query = query;
    this->virtualListVector = new vector<TermVirtualList*>;

    bool returnValue = false;
    const vector<Term*> *queryTerms = query->getQueryTerms();

    for (vector<Term*>::const_iterator vectorIterator = queryTerms->begin();
            vectorIterator != queryTerms->end(); vectorIterator++) {
        // compute the active nodes for this term
        Term *term = *vectorIterator;
        boost::shared_ptr<PrefixActiveNodeSet> termActiveNodeSet = queryEvaluatorInternal
                ->computeActiveNodeSet(term);

        // compute the virtual list for this term
        TermVirtualList *termVirtualList = new TermVirtualList(
                queryEvaluatorInternal->getInvertedIndex(),
                queryEvaluatorInternal->getForwardIndex(),
                termActiveNodeSet.get(),
                term, query->getPrefixMatchPenalty());

        this->virtualListVector->push_back(termVirtualList);
    }
    return returnValue;
}

/*
 * Serialization scheme :
 * | resultsApproximated | estimatedNumberOfResults | sortedFinalResults | facetResults |
 */
void * QueryResultsInternal::serializeForNetwork(void * buffer) const{

	buffer = srch2::util::serializeFixedTypes(resultsApproximated , buffer);
	buffer = srch2::util::serializeFixedTypes(estimatedNumberOfResults , buffer);

	buffer = srch2::util::serializeFixedTypes(unsigned(sortedFinalResults.size()), buffer); // number of query result objects in vector
	for(unsigned queryResultIndex = 0 ; queryResultIndex < sortedFinalResults.size() ; ++queryResultIndex){
		buffer = sortedFinalResults.at(queryResultIndex)->serializeForNetwork(buffer);
	}
	// 	std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > facetResults;
	buffer = srch2::util::serializeFixedTypes(unsigned(facetResults.size()), buffer); // size of map
	for(std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator facetResultItr =
			facetResults.begin() ; facetResultItr != facetResults.end() ; ++facetResultItr){
		buffer = srch2::util::serializeString(facetResultItr->first, buffer); // string
		buffer = srch2::util::serializeFixedTypes(facetResultItr->second.first, buffer); // FacetType
		const std::vector<std::pair<std::string, float> > & vectorToSerialize = facetResultItr->second.second;
		buffer = srch2::util::serializeFixedTypes(unsigned(vectorToSerialize.size()), buffer); // vector size
		for(std::vector<std::pair<std::string, float> >::const_iterator pairItr = vectorToSerialize.begin();
				pairItr != vectorToSerialize.end() ; ++pairItr){
			buffer = srch2::util::serializeString(pairItr->first, buffer); // string
			buffer = srch2::util::serializeFixedTypes(pairItr->second, buffer); // float
		}
	}

	return buffer;

}
/*
 * Serialization scheme :
 * | resultsApproximated | estimatedNumberOfResults | sortedFinalResults | facetResults |
 */
void * QueryResultsInternal::deserializeForNetwork(void * buffer,QueryResultFactory * resultsFactory){

	buffer = srch2::util::deserializeFixedTypes(buffer, resultsApproximated);
	buffer = srch2::util::deserializeFixedTypes(buffer, estimatedNumberOfResults);

	unsigned numberOfResults = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, numberOfResults);
	for(unsigned queryResultIndex = 0 ; queryResultIndex < numberOfResults ; ++queryResultIndex){
		QueryResult * queryResult;
		buffer = QueryResult::deserializeForNetwork(queryResult, buffer,resultsFactory);
		sortedFinalResults.push_back(queryResult);
	}

	// 	std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > facetResults;
	unsigned numberOfFacets = 0 ;
	buffer = srch2::util::deserializeFixedTypes(buffer, numberOfFacets); // size of map
	for(unsigned facetIndex = 0 ; facetIndex < numberOfFacets; ++facetIndex){
		string keyValue;
		buffer = srch2::util::deserializeString(buffer, keyValue); // string
		FacetType facetType;
		buffer = srch2::util::deserializeFixedTypes(buffer,facetType); // FacetType
		std::pair< FacetType , std::vector<std::pair<std::string, float> > > newPair =
				std::make_pair(facetType , std::vector<std::pair<std::string, float> >());
		facetResults[keyValue] = newPair;
		unsigned pairVectorSize = 0;
		buffer = srch2::util::deserializeFixedTypes(buffer, pairVectorSize); // vector size
		for(unsigned vectorElementIndex = 0; vectorElementIndex < pairVectorSize; ++vectorElementIndex){
			facetResults[keyValue].second.push_back(std::make_pair("", 0));
			buffer = srch2::util::deserializeString(buffer , facetResults[keyValue].second.at(facetResults[keyValue].second.size()-1).first); // string
			buffer = srch2::util::deserializeFixedTypes(buffer, facetResults[keyValue].second.at(facetResults[keyValue].second.size()-1).second); // float
		}
	}

	return buffer;
}
/*
 * Serialization scheme :
 * | resultsApproximated | estimatedNumberOfResults | sortedFinalResults | facetResults |
 */
unsigned QueryResultsInternal::getNumberOfBytesForSerializationForNetwork() const{

	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(resultsApproximated);
	numberOfBytes += sizeof(estimatedNumberOfResults);

	numberOfBytes += sizeof(unsigned); // size
	for(unsigned queryResultIndex = 0 ; queryResultIndex < sortedFinalResults.size() ; ++queryResultIndex){
		numberOfBytes += sortedFinalResults.at(queryResultIndex)->getNumberOfBytesForSerializationForNetwork();
	}

	numberOfBytes += sizeof(unsigned); // size
	for(std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator facetResultItr =
			facetResults.begin() ; facetResultItr != facetResults.end() ; ++facetResultItr){
		numberOfBytes += sizeof(unsigned) + facetResultItr->first.size();
		numberOfBytes += sizeof(FacetType);
		const std::vector<std::pair<std::string, float> > & vectorToSerialize = facetResultItr->second.second;
		numberOfBytes += sizeof(unsigned); // vector size
		for(std::vector<std::pair<std::string, float> >::const_iterator pairItr = vectorToSerialize.begin();
				pairItr != vectorToSerialize.end() ; ++pairItr){
			numberOfBytes += sizeof(unsigned) + pairItr->first.size(); // string
			numberOfBytes += sizeof(float); // float
		}
	}

	return numberOfBytes;
}


QueryResultsInternal::~QueryResultsInternal() {
    // TODO: if we use caching, we can leave them in the cache
    if(virtualListVector != NULL){
        for (unsigned int i = 0; i < virtualListVector->size(); ++i) {
            delete virtualListVector->at(i);
        }
        delete virtualListVector;
    }

    sortedFinalResults.clear();
    while (!nextKResultsHeap.empty()) {
        nextKResultsHeap.pop();
    }

    if(this->stat != NULL){
		delete this->stat;
    }

}

void QueryResultsInternal::setNextK(const unsigned k) {
    this->nextK = k;
}

void QueryResultsInternal::insertResult(QueryResult * queryResult) {
    if (this->query->getQueryType() == srch2::instantsearch::SearchTypeTopKQuery) {
        ASSERT(this->nextKResultsHeap.size() <= this->nextK);
        if (this->nextKResultsHeap.size() < this->nextK) {
            this->nextKResultsHeap.push(queryResult);
        } else {
            //TODO
            if (this->nextKResultsHeap.top()->_score < queryResult->_score) {
                this->nextKResultsHeap.pop();
                this->nextKResultsHeap.push(queryResult);
            }
        }
    } else {
        this->nextKResultsHeap.push(queryResult);
    }
}

// if the queue has k results and the min score in the queue >= maxScore, return true
bool QueryResultsInternal::hasTopK(const float maxScoreForUnvisitedRecords) {
    float temp1, temp2;
    if (this->nextKResultsHeap.size() == 0) {
        return false;
    }
    TypedValue tempTypedValue = this->nextKResultsHeap.top()->getResultScore();
    ASSERT(tempTypedValue.getType() == srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT);
    temp1 = tempTypedValue.getFloatTypedValue();
    temp2 = maxScoreForUnvisitedRecords;
    if ((this->nextKResultsHeap.size() == this->nextK) && (temp1 >= temp2))
        return true;

    return false;
}






void QueryResultsInternal::fillVisitedList(set<unsigned> &visitedList) {
    vector<QueryResult *>::const_iterator begin =
            this->sortedFinalResults.begin();
    vector<QueryResult *>::const_iterator end = this->sortedFinalResults.end();

    for (vector<QueryResult *>::const_iterator iterator = begin;
            iterator != end; iterator++) {
        QueryResult * result = *iterator;
        visitedList.insert(result->internalRecordId);
    }
}

void QueryResultsInternal::finalizeResults(const ForwardIndex *forwardIndex) {
//    bool descending = (this->query->getSortableAttributeIdSortOrder()
//            == srch2::instantsearch::SortOrderDescending);
	bool descending = true; // Since we deleted order parameter in configuration but we didn't delete the usage in the code,
								// we change this place to always use descending which means higher scores first.

    int numberOfSortedResults = this->sortedFinalResults.size();

    this->sortedFinalResults.resize(
            numberOfSortedResults + this->nextKResultsHeap.size());
    unsigned tailIndex = numberOfSortedResults + this->nextKResultsHeap.size()
            - 1;
    unsigned index = 0;
    unsigned falseHits = 0; // Deleted Rids

    shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
    forwardIndex->getForwardListDirectory_ReadView(forwardListDirectoryReadView);

    while (this->nextKResultsHeap.size() > 0) {
        string externalRecordId;
        if (forwardIndex->getExternalRecordIdFromInternalRecordId(forwardListDirectoryReadView,
                this->nextKResultsHeap.top()->internalRecordId,
                externalRecordId)) {
            QueryResult * qs = resultsFactory->impl->createQueryResult();
            qs->externalRecordId = externalRecordId;
            qs->internalRecordId = this->nextKResultsHeap.top()
                    ->internalRecordId;
            qs->_score.setTypedValue(this->nextKResultsHeap.top()->_score);
            qs->matchingKeywords.assign(
                    this->nextKResultsHeap.top()->matchingKeywords.begin(),
                    this->nextKResultsHeap.top()->matchingKeywords.end());
            qs->attributeIdsList.assign(
                    this->nextKResultsHeap.top()->attributeIdsList.begin(),
                    this->nextKResultsHeap.top()->attributeIdsList.end());
            qs->editDistances.assign(
                    this->nextKResultsHeap.top()->editDistances.begin(),
                    this->nextKResultsHeap.top()->editDistances.end());
            if (descending)
                this->sortedFinalResults[tailIndex - index] = qs;
            else
                this->sortedFinalResults[index] = qs;
            ++index;
        } else {
            ++falseHits;
        }
        this->nextKResultsHeap.pop();
    }

    if (descending)
        this->sortedFinalResults.erase(
                this->sortedFinalResults.begin() + numberOfSortedResults,
                this->sortedFinalResults.begin() + numberOfSortedResults
                        + falseHits);
    else
        this->sortedFinalResults.erase(
                this->sortedFinalResults.begin() + numberOfSortedResults
                        + index,
                this->sortedFinalResults.begin() + numberOfSortedResults + index
                        + falseHits);

    ASSERT(this->nextKResultsHeap.size() == 0);
}



unsigned QueryResult::getNumberOfBytes(){
	unsigned result = sizeof(QueryResult);
	result += externalRecordId.capacity();
	result += sizeof(exactResultFlag);
	result += _score.getNumberOfBytes() - sizeof(TypedValue);
	for(unsigned i=0 ; i< matchingKeywords.size(); ++i){
		result += matchingKeywords[i].capacity();
	}
	for (unsigned i = 0; i < attributeIdsList.size(); ++i){
		result += attributeIdsList[i].capacity() * sizeof(unsigned);
	}
	result += editDistances.capacity() * sizeof(unsigned);
	result += termTypes.capacity() * sizeof(unsigned);
	result += matchingKeywordTrieNodes.capacity() * sizeof(TrieNodePointer);
	return result;
}

/*
 * Serialization scheme :
 * | internalRecordId | _score | externalRecordId | attributeBitmaps | \
 *   editDistances | termTypes | matchingKeywords | physicalDistance |
 */
void * QueryResult::serializeForNetwork(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(internalRecordId, buffer);
	buffer = srch2::util::serializeFixedTypes(physicalDistance, buffer);
	buffer = _score.serializeForNetwork(buffer);
	buffer = srch2::util::serializeString(externalRecordId, buffer);
	buffer = srch2::util::serializeFixedTypes(exactResultFlag, buffer);
	buffer = srch2::util::serializeVectorOfFixedTypes(attributeIdsList, buffer);
	buffer = srch2::util::serializeVectorOfFixedTypes(editDistances, buffer);
	buffer = srch2::util::serializeVectorOfFixedTypes(termTypes, buffer);
	buffer = srch2::util::serializeVectorOfString(matchingKeywords, buffer);
	buffer = srch2::util::serializeString(inMemoryRecordString, buffer);
	buffer = recordSnippet.serializeForNetwork(buffer);
	buffer = srch2::util::serializeFixedTypes((unsigned)valuesOfParticipatingRefiningAttributes.size(), buffer);
	for(std::map<std::string,TypedValue>::const_iterator attItr =
			valuesOfParticipatingRefiningAttributes.begin(); attItr != valuesOfParticipatingRefiningAttributes.end(); ++attItr){
		buffer = srch2::util::serializeString(attItr->first, buffer);
		buffer = srch2::util::serializeFixedTypes(attItr->second, buffer);
	}
	return buffer;
}
/*
 * Serialization scheme :
 * | physicalDistance | internalRecordId | _score | externalRecordId | attributeBitmaps | \
 *   editDistances | termTypes | matchingKeywords |
 */
void * QueryResult::deserializeForNetwork(QueryResult * &queryResult, void * buffer,QueryResultFactory * resultsFactory){
	queryResult = resultsFactory->impl->createQueryResult();
	buffer = srch2::util::deserializeFixedTypes(buffer, queryResult->internalRecordId);
	buffer = srch2::util::deserializeFixedTypes(buffer, queryResult->physicalDistance);
	buffer = TypedValue::deserializeForNetwork(queryResult->_score, buffer);
	buffer = srch2::util::deserializeString(buffer, queryResult->externalRecordId);
	buffer = srch2::util::deserializeFixedTypes(buffer, queryResult->exactResultFlag);
	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, queryResult->attributeIdsList);
	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, queryResult->editDistances);
	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, queryResult->termTypes);
	buffer = srch2::util::deserializeVectorOfString(buffer, queryResult->matchingKeywords);

	buffer = srch2::util::deserializeString(buffer, queryResult->inMemoryRecordString);
	buffer = RecordSnippet::deserializeForNetwork(buffer, queryResult->recordSnippet);
	unsigned mapSize = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, mapSize);
	for(unsigned i =0 ; i < mapSize; ++i){
		string stringKey;
		TypedValue typedValueValue;
		buffer = srch2::util::deserializeString(buffer, stringKey);
		buffer = srch2::util::deserializeFixedTypes(buffer, typedValueValue);
	}

	return buffer;
}
/*
 * Serialization scheme :
 * | internalRecordId | _score | externalRecordId | attributeBitmaps | \
 *   editDistances | termTypes | matchingKeywords | physicalDistance |
 */
unsigned QueryResult::getNumberOfBytesForSerializationForNetwork() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(internalRecordId);
	numberOfBytes += sizeof(physicalDistance);
	numberOfBytes += _score.getNumberOfBytesForSerializationForNetwork();
	numberOfBytes += sizeof(unsigned) + externalRecordId.size();
	numberOfBytes += sizeof(exactResultFlag);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(attributeIdsList);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(editDistances);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(termTypes);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(matchingKeywords);
	numberOfBytes += sizeof(unsigned) + inMemoryRecordString.size();
	numberOfBytes += recordSnippet.getNumberOfBytesOfSnippets();
	numberOfBytes += sizeof(unsigned);
	for(std::map<std::string,TypedValue>::const_iterator attItr =
			valuesOfParticipatingRefiningAttributes.begin(); attItr != valuesOfParticipatingRefiningAttributes.end(); ++attItr){
		numberOfBytes += sizeof(unsigned) + attItr->first.size();
		numberOfBytes += sizeof(attItr->second);
	}

	return numberOfBytes;
}
}
}
