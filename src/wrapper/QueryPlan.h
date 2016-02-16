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
