
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

#ifndef __WRAPPER_PHYSICALPLAN_H__
#define __WRAPPER_PHYSICALPLAN_H__

#include <vector>

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

typedef const TrieNode* TrieNodePointer;

/*
 * The instance of this structure is the container which moves the needed
 * runtime parameters (like isFuzzy of K in topK) through the physical plan.
 */

struct PhysicalPlanExecutionParameters {
	unsigned k;
	// if this variable is false the operator only returns exact matches by calling getNext(...)
	bool isFuzzy;
	float prefixMatchPenalty ;
	Ranker * ranker;
	PhysicalPlanExecutionParameters(unsigned k,bool isFuzzy,float prefixMatchPenalty,srch2is::QueryType searchType){
		this->k = k;
		this->isFuzzy = isFuzzy ;
		this->prefixMatchPenalty = prefixMatchPenalty;
		switch (searchType) {
			case srch2is::SearchTypeTopKQuery:
				this->ranker = new DefaultTopKRanker();
				break;
			case srch2is::SearchTypeGetAllResultsQuery:
				this->ranker = new GetAllResultsRanker();
				break;
			case srch2is::SearchTypeMapQuery:
				this->ranker = new SpatialRanker();
				break;
			case srch2is::SearchTypeRetrieveById:
				this->ranker = new DefaultTopKRanker();
				break;
		}
	}

	~PhysicalPlanExecutionParameters(){
		if(this->ranker != NULL){
			delete ranker;
		}
	}
};

class PhysicalPlanRecordItem;
/*
 * This structure is used to move information when verifyByRandomAccess() of an
 * operator is called.
 * Other than the recordId to get verified there is more to move.
 * For example, if the record has a match, the information about the matching
 * prefix or the runtime score of the match needs to be returned back to the caller.
 */
struct PhysicalPlanRandomAccessVerificationParameters {
	Ranker * ranker ;
	PhysicalPlanRandomAccessVerificationParameters(Ranker * ranker){
		this->ranker = ranker;
	}

	// if a term is verified, some infomation like staticscore or matching prefix
	// is calculated at that time and will be saved in these variables.
    float runTimeTermRecordScore;
    float staticTermRecordScore;
    std::vector<TrieNodePointer> termRecordMatchingPrefixes;
    std::vector<unsigned> attributeBitmaps;
    std::vector<unsigned> prefixEditDistances;
    std::vector<unsigned> positionIndexOffsets;
    // the record which is going to be verified by forward index
    PhysicalPlanRecordItem * recordToVerify;
    bool isFuzzy;
	float prefixMatchPenalty ;
};

// This class is used to maintain the input/output properties of a PhysicalPlanIterator
class IteratorProperties{
public:
	bool isMatchAsInputTo(const IteratorProperties & prop, IteratorProperties & reason);
	void addProperty(PhysicalPlanIteratorProperty prop);
	vector<PhysicalPlanIteratorProperty> properties;
};

/*
 * This class is the 'tuple' in this iterator model.
 * When the physical plan is being executed, the pointers to
 * PhysicalPlanRecordItem objects are passed around.
 */
class PhysicalPlanRecordItem{
public:
	// getters
	unsigned getRecordId() const ;
	float getRecordStaticScore() const;
	float getRecordRuntimeScore() const;
	void getRecordMatchingPrefixes(vector<TrieNodePointer> & matchingPrefixes) const;
	void getRecordMatchEditDistances(vector<unsigned> & editDistances) const;
	void getRecordMatchAttributeBitmaps(vector<unsigned> & attributeBitmaps) const;
	void getPositionIndexOffsets(vector<unsigned> & positionIndexOffsets)const ;

	// setters
	void setRecordId(unsigned id) ;
	void setRecordStaticScore(float staticScore) ;
	void setRecordRuntimeScore(float runtimeScore) ;
	void setRecordMatchingPrefixes(const vector<TrieNodePointer> & matchingPrefixes) ;
	void setRecordMatchEditDistances(const vector<unsigned> & editDistances) ;
	void setRecordMatchAttributeBitmaps(const vector<unsigned> & attributeBitmaps) ;
	void setPositionIndexOffsets(const vector<unsigned> & positionIndexOffsets);
	~PhysicalPlanRecordItem(){};

    std::map<std::string,TypedValue> valuesOfParticipatingRefiningAttributes;
private:
	unsigned recordId;
	float recordStaticScore;
	float recordRuntimeScore;
	vector<TrieNodePointer> matchingPrefixes;
	vector<unsigned> editDistances;
	vector<unsigned> attributeBitmaps;
	vector<unsigned> positionIndexOffsets;
};

/*
 * The factory class of PhysicalPlanRecordItem;
 */
class PhysicalPlanRecordItemFactory{
public:
	PhysicalPlanRecordItem * createRecordItem(){
		PhysicalPlanRecordItem  * newObj = new PhysicalPlanRecordItem();
		objects.push_back(newObj);
		return newObj;
	}

	~PhysicalPlanRecordItemFactory(){
		for(unsigned i =0 ; i< objects.size() ; ++i){
			if(objects.at(i) == NULL){
				ASSERT(false);
			}else{
				delete objects.at(i);
			}
		}
	}
private:
	vector<PhysicalPlanRecordItem *> objects;
};

// The iterator interface used to implement iterator model
class PhysicalPlanIteratorExecutionInterface{
public:
	virtual bool open(QueryEvaluatorInternal * queryEvaluator,PhysicalPlanExecutionParameters & params) = 0;
	virtual PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) = 0;
	virtual bool close(PhysicalPlanExecutionParameters & params) = 0;

	virtual ~PhysicalPlanIteratorExecutionInterface(){};
};


/*
 * This structure is used to implement the notion of COST of a physical plan.
 * The cost must be estimated so that QueryOptimizer can choose the cheapest plan.
 * The cost of different functions of each operator is estimated by roughly counting the number
 * of instructions done in that function. For example, suppose a function sorts all the records from
 * its child. The cost is :
 * estimatedNumberOfRecordsComingFromChild (R) <- HistogramManager
 *
 * cost = RlogR * some number of instructions  + R * cost of child's getNext
 */
struct PhysicalPlanCost{
	unsigned cost;
	PhysicalPlanCost(){
		cost = 0;
	}
	PhysicalPlanCost(unsigned c){
		cost = c;
	}
	PhysicalPlanCost(const PhysicalPlanCost & src){
		cost = src.cost;
	}
	PhysicalPlanCost operator+(const PhysicalPlanCost & rightValue){
		return PhysicalPlanCost( cost + rightValue.cost);
	}
	PhysicalPlanCost operator+(const unsigned & rightValue){
		return PhysicalPlanCost(cost + rightValue);
	}
	void addInstructionCost(unsigned numberOfInstructions = 1){
		cost += 2 * numberOfInstructions;
	}
	void addFunctionCallCost(unsigned numberOfCalls = 1){
		cost += 3 * numberOfCalls;
	}
	void addSmallFunctionCost(unsigned numberOfCalls = 1){
		addFunctionCallCost(numberOfCalls);
		cost += 10 * numberOfCalls;
	}
	void addMediumFunctionCost(unsigned numberOfCalls = 1){
		addFunctionCallCost(numberOfCalls);
		cost += 50 * numberOfCalls;
	}
	void addLargeFunctionCost(unsigned numberOfCalls = 1){
		addFunctionCallCost(numberOfCalls);
		cost += 100 * numberOfCalls;
	}
};

// The iterator interface used to implement iterator model
class PhysicalPlanIteratorOptimizationInterface{
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	virtual PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) = 0;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	virtual PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) = 0;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	virtual PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) = 0;
	// the cost of verifying a record by random access
	virtual PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params) = 0 ;
	virtual void getOutputProperties(IteratorProperties & prop) = 0;
	virtual void getRequiredInputProperties(IteratorProperties & prop) = 0;
	virtual ~PhysicalPlanIteratorOptimizationInterface(){};
};

class PhysicalPlan;
class PhysicalPlanNode;

/*
 * All optimization nodes inherit from this class.
 * this class has two major capabilities :
 * 1. tree primitives like going to children and counting them and ...
 * 2. cost functions which are used in optimization
 */
class PhysicalPlanOptimizationNode : public PhysicalPlanIteratorOptimizationInterface{
	friend class PhysicalPlan;
public:
	virtual PhysicalPlanNodeType getType() = 0;
	// this function checks the types and properties of children to see if it's
	// meaningful to have this node with this children.
	virtual bool validateChildren() = 0;
	unsigned getChildrenCount() ;
	PhysicalPlanOptimizationNode * getChildAt(unsigned offset) ;
	void setChildAt(unsigned offset, PhysicalPlanOptimizationNode * child) ;
	void addChild(PhysicalPlanOptimizationNode * child) ;
	void setParent(PhysicalPlanOptimizationNode * parent);
	PhysicalPlanOptimizationNode * getParent();
	virtual ~PhysicalPlanOptimizationNode(){}
	void setExecutableNode(PhysicalPlanNode * node){
		this->executableNode = node;
	}
	PhysicalPlanNode * getExecutableNode(){
		return this->executableNode;
	}

	void setLogicalPlanNode(LogicalPlanNode * node){
		this->logicalPlanNode = node;
	}
	LogicalPlanNode * getLogicalPlanNode(){
		return this->logicalPlanNode;
	}

	void printSubTree(unsigned indent = 0);
private:
	PhysicalPlanNode * executableNode;
	vector<PhysicalPlanOptimizationNode *> children;
	// We might want to change the tree to a DAG in future but currently it doesn't make sense
	// since the lowest levels of the tree are the most cost-full parts and it's better not to duplicate keywords
	PhysicalPlanOptimizationNode * parent;

	LogicalPlanNode * logicalPlanNode;
};


/*
 * All execution nodes inherit from this class.
 * Execution nodes are the actual nodes which are executed and produce results.
 * Each execution node is connected to an optimization node which enables it to
 * access it's children.
 */
class PhysicalPlanNode : public PhysicalPlanIteratorExecutionInterface{
	friend class PhysicalPlan;
public:
	void setPhysicalPlanOptimizationNode(PhysicalPlanOptimizationNode * optimizationNode);
	PhysicalPlanOptimizationNode * getPhysicalPlanOptimizationNode();

	// this function checks to see if a record (that its id can be found in parameters) is among the
	// results if this subtree. Different operators have different implementations this function.
	// When verification is performed, some information like match prefix is calculated and saved in
	// members of parameters argument, so if this function returns true, we use parameters members to
	// get that information.
	virtual bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) = 0;
private:
	PhysicalPlanOptimizationNode * optimizationNode;
};




/*
 * Implements the physical plan of the query which will be executed.
 * Just a wrapper which includes the actual physical plan tree (tree) and
 * some more information that must be saved in the physical plan like searchType or parameters container.
 */
class PhysicalPlan{
public:

	PhysicalPlan(QueryEvaluatorInternal * queryEvaluator);
	~PhysicalPlan();


//	ForwardIndex * getForwardIndex();
//	const InvertedIndex * getInvertedIndex();
	PhysicalPlanNode * getPlanTree();
	void setPlanTree(PhysicalPlanNode * tree);
	Ranker * getRanker();
	void setSearchType(srch2is::QueryType searchType);
	srch2is::QueryType getSearchType();
	void setExecutionParameters(PhysicalPlanExecutionParameters * executionParameters);
	PhysicalPlanExecutionParameters * getExecutionParameters();
private:
	QueryEvaluatorInternal * queryEvaluator;
	PhysicalPlanNode * tree;
    srch2is::QueryType searchType;
    PhysicalPlanExecutionParameters * executionParameters;

};


}
}

#endif // __WRAPPER_PHYSICALPLAN_H__
