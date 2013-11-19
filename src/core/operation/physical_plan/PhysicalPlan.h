
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
#include "operation/CatalogManager.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

struct PhysicalPlanExecutionParameters {
	unsigned k;
	// if this variable is true the operator only returns exact matches by calling getNext(...)
	bool exactOnly;
	PhysicalPlanExecutionParameters(unsigned k,bool exactOnly){
		this->k = k;
		this->exactOnly = exactOnly ;
	}
};

// This class is used to maintain the input/output properties of a PhysicalPlanIterator
class IteratorProperties{
public:
	bool isMatchAsInputTo(const IteratorProperties & prop);
	void addProperty(PhysicalPlanIteratorProperty prop);
	vector<PhysicalPlanIteratorProperty> properties;
};

// This class is the ancestor of all different kinds of list items in this iterator model.
// Regardless of what kind of iterator we have, lists are implemented as sequences of PhysicalPlanIterable.
class PhysicalPlanIterable{
	virtual unsigned getRecordId() = 0;
	virtual unsigned getRecordScore() = 0;
	virtual ~PhysicalPlanIterable(){};
	//TODO : maybe more API is required to enable this class to produce a QueryResult object
};

// The iterator interface used to implement iterator model
class PhysicalPlanIterator{
public:
	virtual bool open(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager) = 0;
	virtual PhysicalPlanIterable * getNext(const PhysicalPlanExecutionParameters & params) = 0;
	virtual bool close() = 0;
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	virtual unsigned getCostOfOpen() = 0;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	virtual unsigned getCostOfGetNext() = 0;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	virtual unsigned getCostOfClose() = 0;
	virtual void getOutputProperties(const vector<IteratorProperties> & inputProps, IteratorProperties & prop) = 0;
	virtual void getRequiredInputProperties(IteratorProperties & prop) = 0;
	virtual ~PhysicalPlanIterator();
};

class PhysicalPlan;
class PhysicalPlanNode : public PhysicalPlanIterator{
	friend class PhysicalPlan;
public:
	virtual PhysicalPlanNodeType getType() = 0;
	unsigned getChildrenCount() ;
	PhysicalPlanNode * getChildAt(unsigned offset) ;
	void addChild(PhysicalPlanNode * child) ;
	void setParent(PhysicalPlanNode * parent);
	PhysicalPlanNode * getParent();
private:
	vector<PhysicalPlanNode *> children;
	// We might want to change the tree to a DAG in future but currently it doesn't make sense
	// since the lowest levels of the tree are the most cost-full parts and it's better not to duplicate keywords
	PhysicalPlanNode * parent;

};


/*
 * Implements the physical plan of the query which will be executed.
 * Each node in this plan (PhysicalPlanNode) is a PhysicalPlanIterator and has required API
 * to implement iterator model.
 */
class PhysicalPlan{
public:

	PhysicalPlan(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager);
	~PhysicalPlan();


	PhysicalPlanNode * createNode(PhysicalPlanNodeType nodeType);

	ForwardIndex * getForwardIndex();
	InvertedIndex * getInvertedIndex();
	Trie * getTrie();
	CatalogManager * getCatalogManager();
	PhysicalPlanNode * getPlanTree();

private:
	ForwardIndex * forwardIndex;
	InvertedIndex * invertedIndex;
	Trie * trie;
	CatalogManager * catalogManager;
	PhysicalPlanNode * tree;
};


}
}

#endif // __WRAPPER_PHYSICALPLAN_H__
