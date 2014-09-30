// $Id$ 07/11/13 Jamshid


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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
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
