
#include "instantsearch/QueryEvaluator.h"
#include "IndexerInternal.h"
#include "QueryEvaluatorInternal.h"

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

// for retrieving only one result by having the primary key
void QueryEvaluator::search(const std::string & primaryKey, QueryResults *queryResults){
	this->impl->search(primaryKey , queryResults);
}

void QueryEvaluator::cacheClear() {
	this->impl->cacheClear();
}

}}
