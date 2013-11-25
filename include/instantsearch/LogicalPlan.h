
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

#ifndef __WRAPPER_LOGICALPLAN_H__
#define __WRAPPER_LOGICALPLAN_H__

#include "Constants.h"
#include "instantsearch/Term.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "util/Assert.h"
#include <vector>
#include <string>
using namespace std;

namespace srch2 {
namespace instantsearch {

class LogicalPlanNodeAnnotation;
/*
 * LogicalPlanNode is the common class used for the logical plan tree operators and operands. And tree is
 * constructed by attaching the instances of this class and the pointer to the root is kept in LogicalPlan.
 */
class LogicalPlanNode{
	// Since constructor is private, only LogicalPlan can allocate space for LogicalPlanNodes.
	friend class LogicalPlan;
public:
	LogicalPlanNodeType nodeType;
	vector<LogicalPlanNode *> children;
	Term * exactTerm;
	Term * fuzzyTerm;
	LogicalPlanNodeAnnotation * stats;

	~LogicalPlanNode();

    unsigned getNodeId();

    void setFuzzyTerm(Term * fuzzyTerm);

private:
	unsigned nodeId;
	LogicalPlanNode(Term * exactTerm, Term * fuzzyTerm);
	LogicalPlanNode(LogicalPlanNodeType nodeType);
};


/*
 * LogicalPlan is the class which maintains the logical plan of query and its metadata. The logical plan is
 * a tree of operators (AND,OR and NOT) and query terms. For example, for the query:
 * q=(John AND hello*) OR (authors:Kundera AND freedom~0.5)
 * the logical plan is :
 *
 *   [OR]_____[AND]_____{John}
 *    |         |
 *    |         |_______{hello, TermType=PREFIX}
 *    |
 *    |_______[AND]_____{Kundera, fieldFilter=Authors}
 *              |
 *              |_______{freedom, similarityThreshold=0.5}
 *
 * Each node in this tree is a LogicalPlanNode.
 */
class LogicalPlan {
private:
	// SearchType
	// PostProcessingInfo
    LogicalPlanNode * tree;
    unsigned logicalPlanNodeId;

public:
    LogicalPlan();
    ~LogicalPlan();


	std::string docIdForRetrieveByIdSearchType;
	ResultsPostProcessorPlan * postProcessingPlan;
	/// Plan related information
	srch2::instantsearch::QueryType searchType;
	int offset;
	int resultsToRetrieve;
	bool shouldRunFuzzyQuery;
	Query *exactQuery;
	Query *fuzzyQuery;

    // constructs a term logical plan node
    LogicalPlanNode * createTermLogicalPlanNode(const std::string &queryKeyword,
    		TermType type,
    		const float boost,
    		const float similarityBoost,
    		const uint8_t threshold,
    		unsigned fieldFilter);
    // constructs an internal (operator) logical plan node
    LogicalPlanNode * createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType);

	ResultsPostProcessorPlan* getPostProcessingPlan() const {
		return postProcessingPlan;
	}

	void setPostProcessingPlan(ResultsPostProcessorPlan* postProcessingPlan) {
		this->postProcessingPlan = postProcessingPlan;
	}

	LogicalPlanNode * getTree(){
		return tree;
	}

	const LogicalPlanNode * getTreeForRead() const{
		return tree;
	}
	void setTree(LogicalPlanNode * tree){
		this->tree = tree;
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

	srch2is::QueryType getSearchType() const {
		return searchType;
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

//	// this function translates searchType enum flags to correspondent unsigned values
//	unsigned getSearchTypeCode() const {
//		// TODO : there must be some functions in Config file that give us these codes.
//		switch (getSearchType()) {
//			case srch2http::TopKSearchType:
//				return 0;
//				break;
//			case srch2http::GetAllResultsSearchType:
//				return 1;
//				break;
//			case srch2http::GeoSearchType:
//				return 2;
//				break;
//			case srch2http::RetrieveByIdSearchType:
//				return 3;
//				break;
//			default:
//
//				break;
//		}
//		return 0;
//	}

	void setSearchType(srch2is::QueryType searchType) {
		this->searchType = searchType;
	}

	std::string getDocIdForRetrieveByIdSearchType(){
		return this->docIdForRetrieveByIdSearchType;
	}

	void setDocIdForRetrieveByIdSearchType(const std::string & docid){
		this->docIdForRetrieveByIdSearchType = docid;
	}

};

}
}

#endif // __WRAPPER_LOGICALPLAN_H__
