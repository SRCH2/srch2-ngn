#ifndef _WRAPPER_QUERYPLEAN_H_
#define _WRAPPER_QUERYPLEAN_H_

#include "instantsearch/ResultsPostProcessor.h"
#include "ParsedParameterContainer.h"
#include "util/Assert.h"

using srch2::instantsearch::ResultsPostProcessorPlan;
using srch2::instantsearch::Query;

using namespace std;
namespace srch2{

namespace httpwrapper{

class QueryPlan
{
public:


    QueryPlan(){
        exactQuery = NULL;
        fuzzyQuery = NULL;
        postProcessingPlan = NULL;
    }
	~QueryPlan(){
		if(exactQuery != NULL) delete exactQuery;

		if(fuzzyQuery != NULL) delete fuzzyQuery;

		if(postProcessingPlan != NULL) delete postProcessingPlan;

	}
	Query* getExactQuery() {
		return exactQuery;
	}

	void setExactQuery(Query* exactQuery) { // TODO : change the header
		// it gets enough information from the arguments and allocates the query objects
        if(this->exactQuery == NULL){
            this->exactQuery = exactQuery;
        }
	}

	Query* getFuzzyQuery() {
		return fuzzyQuery;
	}

	void setFuzzyQuery(Query* fuzzyQuery) { // TODO : change the header

		// it gets enough information from the arguments and allocates the query objects
	    if(this->fuzzyQuery == NULL){
            this->fuzzyQuery = fuzzyQuery;
	    }
	}


	ResultsPostProcessorPlan* getPostProcessingPlan() const {
		return postProcessingPlan;
	}

	void setPostProcessingPlan(ResultsPostProcessorPlan* postProcessingPlan) {
		this->postProcessingPlan = postProcessingPlan;
	}

	bool isFuzzy() const {
		return shouldRunFuzzyQuery;
	}

	void setFuzzy(bool isFuzzy) {
		this->shouldRunFuzzyQuery = isFuzzy;
	}

	int getOffset() const {
		return offset;
	}

	void setOffset(int offset) {
		this->offset = offset;
	}

	int getResultsToRetrieve() const {
		return resultsToRetrieve;
	}

	void setResultsToRetrieve(int resultsToRetrieve) {
		this->resultsToRetrieve = resultsToRetrieve;
	}

	ParameterName getSearchType() const {
		return searchType;
	}

	void setSearchType(ParameterName searchType) {
		this->searchType = searchType;
	}

	std::string getDocIdForRetrieveByIdSearchType(){
		return this->docIdForRetrieveByIdSearchType;
	}

	void setDocIdForRetrieveByIdSearchType(const std::string & docid){
		this->docIdForRetrieveByIdSearchType = docid;
	}


private:

	std::string docIdForRetrieveByIdSearchType;

	Query *exactQuery;
	Query *fuzzyQuery;

	ResultsPostProcessorPlan * postProcessingPlan;


	/// Plan related information

	ParameterName searchType;
	int offset;
	int resultsToRetrieve;
	bool shouldRunFuzzyQuery;

};
}
}

#endif // _WRAPPER_QUERYPLEAN_H_
