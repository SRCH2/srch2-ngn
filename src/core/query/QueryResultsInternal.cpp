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

#include "QueryResultsInternal.h"
#include "operation/TermVirtualList.h"
#include "operation/QueryEvaluatorInternal.h"
#include "util/Logger.h"
#include <instantsearch/Query.h>
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

}
}
