
// $Id: QueryResultsInternal.cpp 3480 2013-06-19 08:00:34Z jiaying $

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

#include "QueryResultsInternal.h"
#include "operation/TermVirtualList.h"
//#include "index/Trie.h"
#include "operation/IndexSearcherInternal.h"
#include "util/Logger.h"
#include <instantsearch/Query.h>
//#include <instantsearch/Term.h>
#include <sstream>

using std::vector;
using srch2::util::Logger;

namespace srch2
{
namespace instantsearch
{
class Term;
QueryResultsInternal::QueryResultsInternal(IndexSearcherInternal *indexSearcherInternal, Query *query)
{
    this->query = query;
    this->virtualListVector = new vector<TermVirtualList* >;
    this->indexSearcherInternal = indexSearcherInternal;
    
    this->stat = new Stat();
}
    
// DEBUG function. Used in CacheIntegration_Test
bool QueryResultsInternal::checkCacheHit(IndexSearcherInternal *indexSearcherInternal, Query *query)
{
    this->query = query;
    this->virtualListVector = new vector<TermVirtualList* >;

    bool returnValue = false;
    const vector<Term* > *queryTerms = query->getQueryTerms();
    
    for (vector<Term*>::const_iterator vectorIterator = queryTerms->begin();
     vectorIterator != queryTerms->end(); vectorIterator++) {
        // compute the active nodes for this term
        Term *term = *vectorIterator;
        PrefixActiveNodeSet *termActiveNodeSet = indexSearcherInternal->computeActiveNodeSet(term);

        // compute the virtual list for this term
        TermVirtualList *termVirtualList = new TermVirtualList(indexSearcherInternal->getInvertedIndex(), 
                                       termActiveNodeSet, term, query->getPrefixMatchPenalty());

        // check if termActiveNodeSet is cached, if not delete it to prevent memory leaks.
        if (termActiveNodeSet->isResultsCached() == false) {
            delete termActiveNodeSet;
        }
        else {
            returnValue = true;
        }
    
        this->virtualListVector->push_back(termVirtualList);
    }
    return returnValue;
}

QueryResultsInternal::~QueryResultsInternal()
{
    // TODO: if we use caching, we can leave them in the cache
    for (unsigned int i = 0; i < virtualListVector->size(); ++i) {
        delete virtualListVector->at(i);
    }
    delete virtualListVector;
    
    sortedFinalResults.clear();
    while (!nextKResultsHeap.empty()) {
        nextKResultsHeap.pop();
    }
    
    delete this->stat;
}

unsigned QueryResultsInternal::getNumberOfResults() const
{
    return this->sortedFinalResults.size();
}

std::string QueryResultsInternal::getRecordId(unsigned position) const
{
    ASSERT (position < this->getNumberOfResults());
    return this->sortedFinalResults.at(position).externalRecordId;
}

unsigned QueryResultsInternal::getInternalRecordId(unsigned position) const
{
    ASSERT (position < this->getNumberOfResults());
    return this->sortedFinalResults.at(position).internalRecordId;
}

std::string QueryResultsInternal::getInMemoryRecordString(unsigned position) const
{
    unsigned internalRecordId = this->getInternalRecordId(position);
    return this->indexSearcherInternal->getInMemoryData(internalRecordId);
}

float QueryResultsInternal::getResultScore(unsigned position) const
{
    ASSERT (position < this->getNumberOfResults());
    return this->sortedFinalResults.at(position).score;
}

double QueryResultsInternal::getPhysicalDistance(const unsigned position) const
{
    ASSERT (position < this->getNumberOfResults());
    return this->sortedFinalResults.at(position).physicalDistance;
}

void QueryResultsInternal::getMatchingKeywords(const unsigned position, vector<string> &matchingKeywords) const
{
    matchingKeywords.assign(this->sortedFinalResults[position].matchingKeywords.begin(),
    		this->sortedFinalResults[position].matchingKeywords.end());
}
    
void QueryResultsInternal::getEditDistances(const unsigned position, vector<unsigned> &editDistances) const
{
    editDistances.assign(this->sortedFinalResults[position].editDistances.begin(),
    		this->sortedFinalResults[position].editDistances.end());
}

void QueryResultsInternal::getMatchedAttributeBitmaps(const unsigned position, std::vector<unsigned> &matchedAttributeBitmaps) const
{
	matchedAttributeBitmaps.assign(this->sortedFinalResults[position].attributeBitmaps.begin(),
			this->sortedFinalResults[position].attributeBitmaps.end());
}


// return the matchedAttributes indexed from 0
void QueryResultsInternal::getMatchedAttributes(const unsigned position, std::vector<std::vector<unsigned> > &matchedAttributes) const
{
	//TODO opt
	const vector<unsigned> &matchedAttributeBitmaps = this->sortedFinalResults[position].attributeBitmaps;
	matchedAttributes.resize(matchedAttributeBitmaps.size());

	for(int i = 0; i < matchedAttributeBitmaps.size(); i++)
	{
		unsigned idx = 0;
		unsigned matchedAttributeBitmap = matchedAttributeBitmaps[i];
		matchedAttributes[i].clear();
		while(matchedAttributeBitmap)
		{
			if(matchedAttributeBitmap & 0x1)
			{
				matchedAttributes[i].push_back(idx);
			}
			matchedAttributeBitmap >>= 1;
			++idx;
		}
	}
}

void QueryResultsInternal::setNextK(const unsigned k)
{
    this->nextK = k;
}

void QueryResultsInternal::insertResult(QueryResult &queryResult)
{
    if (this->query->getQueryType() == srch2::instantsearch::TopKQuery) {
        ASSERT(this->nextKResultsHeap.size() <= this->nextK);
        if (this->nextKResultsHeap.size() < this->nextK) {
            this->nextKResultsHeap.push(queryResult);
        }
        else {
            if (this->nextKResultsHeap.top().score < queryResult.score) {
                this->nextKResultsHeap.pop();
                this->nextKResultsHeap.push(queryResult);
            }
        }
    }
    else {
        this->nextKResultsHeap.push(queryResult);
    }
}

// if the queue has k results and the min score in the queue >= maxScore, return true
bool QueryResultsInternal::hasTopK(const float maxScoreForUnvisitedRecords)
{
    if ((this->nextKResultsHeap.size() == this->nextK)
    && (this->nextKResultsHeap.top().score >= maxScoreForUnvisitedRecords))
        return true;

  return false;
}

void QueryResultsInternal::fillVisitedList(set<unsigned> &visitedList)
{
    vector<QueryResult>::const_iterator begin = this->sortedFinalResults.begin();
    vector<QueryResult>::const_iterator end = this->sortedFinalResults.end();

    for (vector<QueryResult>::const_iterator iterator = begin; iterator != end; iterator++) {
        visitedList.insert(iterator->internalRecordId);
    }
}

void QueryResultsInternal::finalizeResults(const ForwardIndex *forwardIndex)
{
    bool descending = (this->query->getSortableAttributeIdSortOrder() == srch2::instantsearch::Descending);

    int numberOfSortedResults = this->getNumberOfResults();

    this->sortedFinalResults.resize(numberOfSortedResults+this->nextKResultsHeap.size());
    unsigned tailIndex = numberOfSortedResults+this->nextKResultsHeap.size() - 1;
    unsigned index = 0;
    unsigned falseHits = 0; // Deleted Rids

    while (this->nextKResultsHeap.size() > 0) {
        string externalRecordId;
        if (forwardIndex->getExternalRecordId_ReadView(this->nextKResultsHeap.top().internalRecordId, 
                               externalRecordId)) {
            QueryResult qs;
            qs.externalRecordId = externalRecordId;
            qs.internalRecordId = this->nextKResultsHeap.top().internalRecordId;
            qs.score = this->nextKResultsHeap.top().score;
            qs.matchingKeywords.assign(this->nextKResultsHeap.top().matchingKeywords.begin(),
                           this->nextKResultsHeap.top().matchingKeywords.end());
            qs.attributeBitmaps.assign(this->nextKResultsHeap.top().attributeBitmaps.begin(),
            				this->nextKResultsHeap.top().attributeBitmaps.end());
            qs.editDistances.assign(this->nextKResultsHeap.top().editDistances.begin(),
                           this->nextKResultsHeap.top().editDistances.end());
            if (descending)
                this->sortedFinalResults[tailIndex-index] = qs;
            else
                this->sortedFinalResults[index] = qs;
            ++index;
        }
        else {
            ++falseHits;
        }
        this->nextKResultsHeap.pop();
    }

    if (descending)
        this->sortedFinalResults.erase(this->sortedFinalResults.begin() + numberOfSortedResults, 
                                       this->sortedFinalResults.begin() + numberOfSortedResults + falseHits);
    else
        this->sortedFinalResults.erase(this->sortedFinalResults.begin() + numberOfSortedResults + index, 
                                       this->sortedFinalResults.begin() + numberOfSortedResults + index + falseHits);

    ASSERT(this->nextKResultsHeap.size() == 0);
}

void QueryResultsInternal::printStats() const
{
    this->stat->print();
}

void QueryResultsInternal::printResult() const
{
	// show attributeBitmaps
	vector<unsigned> attributeBitmaps;
	vector<vector<unsigned> > attributes;
	vector<string> matchedKeywords;
    Logger::debug("Result count %d" ,this->getNumberOfResults());
	for(int i = 0; i < this->getNumberOfResults(); i++)
	{
        Logger::debug("Result #%d" ,i);
		this->getMatchedAttributeBitmaps(i, attributeBitmaps);
		this->getMatchingKeywords(i, matchedKeywords);
		this->getMatchedAttributes(i, attributes);
		for(int j = 0; j < attributeBitmaps.size(); j++)
		{
            Logger::debug("%s %d {", matchedKeywords[j].c_str(), attributeBitmaps[j]);
			for(int k = 0; k < attributes[j].size(); k++)
                Logger::debug("%d", attributes[j][k]);
            Logger::debug("}");
		}

	}

}


}}
