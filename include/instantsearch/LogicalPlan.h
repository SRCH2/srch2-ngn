
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
#include "record/LocationRecordUtil.h"
#include <vector>
#include <string>
#include <sstream>
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
	LogicalPlanNode(const LogicalPlanNode & node);

	LogicalPlanNodeType nodeType;
	// this flag is used in case we want to force Query Optimizer to use a specific physical node.
	// currently it's used for Suggestion
	PhysicalPlanNodeType forcedPhysicalNode;
	Term * exactTerm;
	Term * fuzzyTerm;
	vector<LogicalPlanNode *> children;
	Shape* regionShape;
	LogicalPlanNodeAnnotation * stats;


	virtual ~LogicalPlanNode();

    void setFuzzyTerm(Term * fuzzyTerm);

    unsigned getNumberOfLeafNodes(){
    	if(nodeType == LogicalPlanNodeTypeTerm){
    		return 1;
    	}else{
    		unsigned sumOfChildren = 0;
    		for(vector<LogicalPlanNode *>::iterator child = children.begin(); child != children.end() ; ++child){
    			sumOfChildren += (*child)->getNumberOfLeafNodes();
    		}
    		return sumOfChildren;
    	}
    }

    virtual string toString();
    string getSubtreeUniqueString();

    /*
     * Serialization scheme:
     * | nodeType | forcedPhysicalNode | isNULL | isNULL | [exactTerm] | [fuzzyTerm] | children |
     * NOTE : stats is NULL until logical plan reaches to the core so we don't serialize it...
     */
    void * serializeForNetwork(void * buffer);
    static void * deserializeForNetwork(LogicalPlanNode * & node, void * buffer);
    unsigned getNumberOfBytesForSerializationForNetwork();
    Term * getTerm(bool isFuzzy){
    	if(isFuzzy){
    		return this->fuzzyTerm;
    	}else{
    		return this->exactTerm;
    	}
    }

protected:
	LogicalPlanNode(Term * exactTerm, Term * fuzzyTerm);
	LogicalPlanNode(LogicalPlanNodeType nodeType);
private:
	LogicalPlanNode();
	LogicalPlanNode(Shape* regionShape);
};

class LogicalPlanPhraseNode : public LogicalPlanNode{
public:
	LogicalPlanPhraseNode(const vector<string>& phraseKeyWords,
	    		const vector<unsigned>& phraseKeywordsPosition,
	    		short slop, const vector<unsigned>& fieldFilter,
                        ATTRIBUTES_OP attrOp) : LogicalPlanNode(LogicalPlanNodeTypePhrase) {
		phraseInfo = new PhraseInfo();
		phraseInfo->attributeIdsList = fieldFilter;
		phraseInfo->phraseKeyWords = phraseKeyWords;
		phraseInfo->phraseKeywordPositionIndex = phraseKeywordsPosition;
		phraseInfo->proximitySlop = slop;
		phraseInfo->attrOps = attrOp; 
	}


	LogicalPlanPhraseNode(const LogicalPlanPhraseNode & node):
	LogicalPlanNode(node){
		this->phraseInfo = new PhraseInfo(*(node.phraseInfo));
	}

	/*
	 * this constructor makes an empty object for deserialization
	 */
	LogicalPlanPhraseNode() : LogicalPlanNode(LogicalPlanNodeTypePhrase) {
		phraseInfo = new PhraseInfo();
	}
	string toString() {
		string key = LogicalPlanNode::toString();
		if (phraseInfo) {
			key += phraseInfo->toString();
		}
		return key;
	}
	~LogicalPlanPhraseNode() { delete phraseInfo;}
	PhraseInfo * getPhraseInfo() { return phraseInfo; }
private:
	PhraseInfo *phraseInfo;
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
    vector<string> attributesToReturn;

public:
    LogicalPlan();
    LogicalPlan(const LogicalPlan & logicalPlan);

    ~LogicalPlan();

    void setAttrToReturn(const vector<string>& attr){
        attributesToReturn = attr;
    }

    const vector<string> getAttrToReturn() const{
        return this->attributesToReturn;
    }

	// the offset of requested results in the result set
	int offset;
	int numberOfResultsToRetrieve;
	bool shouldRunFuzzyQuery;
	srch2::instantsearch::QueryType queryType;
	std::string docIdForRetrieveByIdSearchType;
	Query *exactQuery;
	Query *fuzzyQuery;
	/// Plan related information
	ResultsPostProcessorPlan * postProcessingPlan;
	ResultsPostProcessingInfo * postProcessingInfo;

    // constructs a term logical plan node
    LogicalPlanNode * createTermLogicalPlanNode(const std::string &queryKeyword,
    		TermType type,
    		const float boost,
    		const float similarityBoost,
    		const uint8_t threshold,
    		const vector<unsigned>& fieldFilter, ATTRIBUTES_OP atrOps);
    // constructs an internal (operator) logical plan node
    LogicalPlanNode * createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType);
    LogicalPlanNode * createPhraseLogicalPlanNode(const vector<string>& phraseKeyWords,
    		const vector<unsigned>& phraseKeywordsPosition,
    		short slop, const vector<unsigned>& fieldFilter, ATTRIBUTES_OP attrOp) ;
	LogicalPlanNode * createGeoLogicalPlanNode(Shape *regionShape);

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


	/*
	 * This function returns a string representation of the logical plan
	 * by concatenating different parts together. The call to getSubtreeUniqueString()
	 * gives us a tree representation of the logical plan tree. For example is query is
	 * q = FOO1 AND BAR OR FOO2
	 * the string of this subtree is something like:
	 * FOO1_BAR_FOO2_OR_AND
	 */
	string getUniqueStringForCaching();

	/*
	 * Serialization scheme :
	 * | offset | numberOfResultsToRetrieve | shouldRunFuzzyQuery | queryType | \
	 *  docIdForRetrieveByIdSearchType | isNULL | isNULL | isNULL | isNULL | \
	 *   [exactQuery] | [fuzzyQuery] | [postProcessingInfo] | [tree] |
	 */
	void * serializeForNetwork(void * buffer);
	static void * deserializeForNetwork(LogicalPlan & logicalPlan , void * buffer, const Schema * schema);
	unsigned getNumberOfBytesForSerializationForNetwork();
};

}
}

#endif // __WRAPPER_LOGICALPLAN_H__
