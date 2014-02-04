
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

#include "instantsearch/QueryEvaluator.h"
#include "IndexerInternal.h"
#include "QueryEvaluatorInternal.h"
//#include "Cache.h"

namespace srch2
{
namespace instantsearch
{

/**
 * Creates an QueryEvaluator object.
 * @param indexer - An object holding the index structures and cache.
 */
QueryEvaluator::QueryEvaluator(Indexer *indexer , QueryEvaluatorRuntimeParametersContainer * parameters ){
	this->impl = new QueryEvaluatorInternal(dynamic_cast<IndexReaderWriter *>(indexer), parameters);
}

QueryEvaluator::~QueryEvaluator(){
	delete this->impl;
}

/*
 * Finds the suggestions for a keyword based on fuzzyMatchPenalty.
 * Returns the number of suggestions found.
 */
// TODO : FIXME: This function is not compatible with the new api
int QueryEvaluator::suggest(const string & keyword, float fuzzyMatchPenalty , const unsigned numberOfSuggestionsToReturn , vector<string> & suggestions ){
	return this->impl->suggest(keyword , fuzzyMatchPenalty , numberOfSuggestionsToReturn, suggestions);
}

/*
 * Returns the estimated number of results
 */
unsigned QueryEvaluator::estimateNumberOfResults(const LogicalPlan * logicalPlan){
	return this->impl->estimateNumberOfResults(logicalPlan);
}

/**
 * If the search type is set to TopK in LogicalPlan, this function
 * finds the next topK answers starting from
 * offset. This function can be used to support pagination of
 * search results. queryResults is a QueryResults
 * object, which must be created using the same query object
 * as the first argument in this function.
 *
 * returns the number of records found (at most topK).
 *
 * if search type is getAllResults, this function finds all the results.
 */
int QueryEvaluator::search(LogicalPlan * logicalPlan , QueryResults *queryResults){
	return this->impl->search(logicalPlan , queryResults);
}

/**
 * Does Map Search
 */
int QueryEvaluator::geoSearch(const Query *query, QueryResults *queryResults){
	return this->impl->geoSearch(query , queryResults);
}

// for doing a geo range query with a circle
void QueryEvaluator::geoSearch(const Circle &queryCircle, QueryResults *queryResults){
	this->impl->geoSearch(queryCircle , queryResults);
}

// for doing a geo range query with a rectangle
void QueryEvaluator::geoSearch(const Rectangle &queryRectangle, QueryResults *queryResults){
	this->impl->geoSearch(queryRectangle , queryResults);
}

// for retrieving only one result by having the primary key
void QueryEvaluator::search(const std::string & primaryKey, QueryResults *queryResults){
	this->impl->search(primaryKey , queryResults);
}

/// Get the in memory data stored with the record in the forwardindex. Access through the internal recordid.
StoredRecordBuffer QueryEvaluator::getInMemoryData(unsigned internalRecordId) const {
	return this->impl->getInMemoryData(internalRecordId);
}

void QueryEvaluator::cacheClear() {
	this->impl->cacheClear();
}

}}
