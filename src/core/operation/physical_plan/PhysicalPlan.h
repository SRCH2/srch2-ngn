
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

struct PhysicalPlanExecutionParameters {
	unsigned k;
	// if this variable is false the operator only returns exact matches by calling getNext(...)
	bool isFuzzy;
	PhysicalPlanExecutionParameters(unsigned k,bool exactOnly){
		this->k = k;
		this->isFuzzy = exactOnly ;
	}
};

struct PhysicalPlanRandomAccessVerificationParameters {

};

// This class is used to maintain the input/output properties of a PhysicalPlanIterator
class IteratorProperties{
public:
	bool isMatchAsInputTo(const IteratorProperties & prop, IteratorProperties & reason);
	void addProperty(PhysicalPlanIteratorProperty prop);
	vector<PhysicalPlanIteratorProperty> properties;
};

// This class is the ancestor of all different kinds of list items in this iterator model.
// Regardless of what kind of iterator we have, lists are implemented as sequences of PhysicalPlanIterable.
class PhysicalPlanRecordItem{
	// getters
	virtual unsigned getRecordId() = 0;
	virtual float getRecordStaticScore() = 0;
	virtual float getRecordRuntimeScore() = 0;
	virtual void getRecordMatchingPrefixes(vector<TrieNodePointer> & matchingPrefixes) = 0;
	virtual void getRecordMatchEditDistances(vector<unsigned> & editDistances) = 0;
	virtual void getRecordMatchAttributeBitmaps(vector<unsigned> & attributeBitmaps) = 0;

	// setters
	virtual void setRecordId(unsigned id) = 0;
	virtual void setRecordStaticScore(float staticScore) = 0;
	virtual void setRecordRuntimeScore(float runtimeScore) = 0;
	virtual void setRecordMatchingPrefixes(const vector<TrieNodePointer> & matchingPrefixes) = 0;
	virtual void setRecordMatchEditDistances(const vector<unsigned> & editDistances) = 0;
	virtual void setRecordMatchAttributeBitmaps(const vector<unsigned> & attributeBitmaps) = 0;
	virtual ~PhysicalPlanRecordItem(){};
};

// The iterator interface used to implement iterator model
class PhysicalPlanIteratorExecutionInterface{
public:
	virtual bool open(QueryEvaluatorInternal * queryEvaluator,PhysicalPlanExecutionParameters & params) = 0;
	virtual PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) = 0;
	virtual bool close(PhysicalPlanExecutionParameters & params) = 0;

	virtual ~PhysicalPlanIteratorExecutionInterface(){};
};

// The iterator interface used to implement iterator model
class PhysicalPlanIteratorOptimizationInterface{
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	virtual unsigned getCostOfOpen(const PhysicalPlanExecutionParameters & params) = 0;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	virtual unsigned getCostOfGetNext(const PhysicalPlanExecutionParameters & params) = 0;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	virtual unsigned getCostOfClose(const PhysicalPlanExecutionParameters & params) = 0;
	virtual void getOutputProperties(IteratorProperties & prop) = 0;
	virtual void getRequiredInputProperties(IteratorProperties & prop) = 0;
	virtual ~PhysicalPlanIteratorOptimizationInterface(){};
};

class PhysicalPlan;
class PhysicalPlanNode;

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

class PhysicalPlanNode : public PhysicalPlanIteratorExecutionInterface{
	friend class PhysicalPlan;
public:
	void setPhysicalPlanOptimizationNode(PhysicalPlanOptimizationNode * optimizationNode);
	PhysicalPlanOptimizationNode * getPhysicalPlanOptimizationNode();

	virtual bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) = 0;
private:
	PhysicalPlanOptimizationNode * optimizationNode;
};




/*
 * Implements the physical plan of the query which will be executed.
 * Each node in this plan (PhysicalPlanNode) is a PhysicalPlanIterator and has required API
 * to implement iterator model.
 */
class PhysicalPlan{
public:

	PhysicalPlan(QueryEvaluatorInternal * queryEvaluator);
	~PhysicalPlan();


	PhysicalPlanNode * createNode(PhysicalPlanNodeType nodeType);

	ForwardIndex * getForwardIndex();
	const InvertedIndex * getInvertedIndex();
	const Trie * getTrie();
	PhysicalPlanNode * getPlanTree();
	void setPlanTree(PhysicalPlanNode * tree);
	Ranker * getRanker();
	void setSearchTypeAndRanker(srch2is::QueryType searchType);
	srch2is::QueryType getSearchType();
	void setExecutionParameters(PhysicalPlanExecutionParameters * executionParameters);
	PhysicalPlanExecutionParameters * getExecutionParameters();
private:
	QueryEvaluatorInternal * queryEvaluator;
	PhysicalPlanNode * tree;
    Ranker *ranker;
    srch2is::QueryType searchType;
    PhysicalPlanExecutionParameters * executionParameters;

};


}
}

#endif // __WRAPPER_PHYSICALPLAN_H__
