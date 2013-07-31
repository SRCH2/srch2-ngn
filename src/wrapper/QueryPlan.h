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

using srch2::instantsearch::ResultsPostProcessorPlan;
using srch2::instantsearch::Query;

using namespace std;
namespace srch2{

namespace httpwrapper{

class QueryPlan
{
public:


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
		this->exactQuery = exactQuery;
	}

	Query* getFuzzyQuery() {
		return fuzzyQuery;
	}

	void setFuzzyQuery(Query* fuzzyQuery) { // TODO : change the header

		// it gets enough information from the arguments and allocates the query objects
		this->fuzzyQuery = fuzzyQuery;
	}


	ResultsPostProcessorPlan* getPostProcessingPlan() const {
		return postProcessingPlan;
	}

	void setPostProcessingPlan(ResultsPostProcessorPlan* postProcessingPlan) {
		this->postProcessingPlan = postProcessingPlan;
	}

	bool isIsFuzzy() const {
		return isFuzzy;
	}

	void setIsFuzzy(bool isFuzzy) {
		this->isFuzzy = isFuzzy;
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


	// this function translates searchType enum flags to correspondent unsigned values
	unsigned getSearchTypeCode() const {
		// TODO : there must be some functions in Config file that give us these codes.
		switch (getSearchType()) {
			case srch2http::TopKSearchType:
				return 0;
				break;
			case srch2http::GetAllResultsSearchType:
				return 1;
				break;
			case srch2http::GeoSearchType:
				return 2;
				break;
			default:

				break;
		}
		return 0;
	}

	void setSearchType(ParameterName searchType) {
		this->searchType = searchType;
	}


private:

	Query *exactQuery;
	Query *fuzzyQuery;

	ResultsPostProcessorPlan * postProcessingPlan;


	/// Plan related information

	ParameterName searchType;
	int offset;
	int resultsToRetrieve;
	bool isFuzzy;

	///








};
}
}

#endif // _WRAPPER_QUERYPLEAN_H_
