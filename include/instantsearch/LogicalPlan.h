
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

	PhysicalPlanNodeType forcedPhysicalNode;

	~LogicalPlanNode();

    void setFuzzyTerm(Term * fuzzyTerm);

    Term * getTerm(bool isFuzzy){
    	if(isFuzzy){
    		return this->fuzzyTerm;
    	}else{
    		return this->exactTerm;
    	}
    }

private:
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
 * Note: The shapes of logical plan and parse tree are similar. The difference between parse-tree and
 * logical plan is:
 * 1. Logical plan is made by traversing the parse tree. So parse tree is a product which is made before
 *    logical plan from the query
 * 2. LogicalPlan objects keep other information like searchType in addition to the query tree
 */
class LogicalPlan {
private:
	// SearchType
	// PostProcessingInfo
    LogicalPlanNode * tree;

public:
    LogicalPlan();
    ~LogicalPlan();


	std::string docIdForRetrieveByIdSearchType;
	ResultsPostProcessorPlan * postProcessingPlan;
	ResultsPostProcessingInfo * postProcessingInfo;
	/// Plan related information
	srch2::instantsearch::QueryType queryType;
	// the offset of requested results in the result set
	int offset;
	int numberOfResultsToRetrieve;
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

	ResultsPostProcessingInfo* getPostProcessingInfo() {
		return postProcessingInfo;
	}

	void setPostProcessingInfo(ResultsPostProcessingInfo* postProcessingInfo) {
		this->postProcessingInfo = postProcessingInfo;
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

	// if this function returns true we must use fuzzy search
	// if we dont find enough exact results;
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

	int getNumberOfResultsToRetrieve() const {
		return numberOfResultsToRetrieve;
	}

	void setNumberOfResultsToRetrieve(int resultsToRetrieve) {
		this->numberOfResultsToRetrieve = resultsToRetrieve;
	}

	srch2is::QueryType getQueryType() const {
		return queryType;
	}
	Query* getExactQuery() const{
		return exactQuery;
	}

	void setExactQuery(Query* exactQuery) {
		// it gets enough information from the arguments and allocates the query object
        if(this->exactQuery == NULL){
            this->exactQuery = exactQuery;
        }else{
        	ASSERT(false);
        }
	}

	Query* getFuzzyQuery() {
		return fuzzyQuery;
	}

	void setFuzzyQuery(Query* fuzzyQuery) {

		// it gets enough information from the arguments and allocates the query object
	    if(this->fuzzyQuery == NULL){
            this->fuzzyQuery = fuzzyQuery;
	    }else{
	    	ASSERT(false);
	    }
	}

	void setQueryType(srch2is::QueryType queryType) {
		this->queryType = queryType;
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
