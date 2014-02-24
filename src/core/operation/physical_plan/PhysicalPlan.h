
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


class PhysicalOperatorCacheObject {
public:
	// this class is only used to implement polymorphism

	vector<PhysicalOperatorCacheObject * > children;

    virtual unsigned getNumberOfBytes() = 0;

	virtual ~PhysicalOperatorCacheObject(){
		for(unsigned childOffset = 0 ; childOffset < children.size() ; ++childOffset){
			if(children.at(childOffset) != NULL){
				delete children.at(childOffset);
			}
		}
	}
};

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
	// If this member is true, then cacheObject contains a cache feed from parent
	bool parentIsCacheEnabled;
	PhysicalOperatorCacheObject * cacheObject ;
	unsigned totalNumberOfRecords;

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

		cacheObject = NULL;
		parentIsCacheEnabled = false;
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
	PhysicalPlanRandomAccessVerificationParameters(Ranker * ranker,
			shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView):
				forwardListDirectoryReadView(forwardListDirectoryReadView)
	{
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
    shared_ptr<vectorview<ForwardListPtr> > & forwardListDirectoryReadView;
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
	inline unsigned getRecordId() const {
		return this->recordId;
	}
	inline float getRecordStaticScore() const{
		return this->recordStaticScore;
	}
	inline float getRecordRuntimeScore() const{
		return this->recordRuntimeScore;
	}
	inline void getRecordMatchingPrefixes(vector<TrieNodePointer> & matchingPrefixes) const{
		matchingPrefixes.insert(matchingPrefixes.end(),this->matchingPrefixes.begin(),this->matchingPrefixes.end());
	}
	inline void getRecordMatchEditDistances(vector<unsigned> & editDistances) const{
		editDistances.insert(editDistances.end(),this->editDistances.begin(),this->editDistances.end());
	}
	inline void getRecordMatchAttributeBitmaps(vector<unsigned> & attributeBitmaps) const{
		attributeBitmaps.insert(attributeBitmaps.end(),this->attributeBitmaps.begin(),this->attributeBitmaps.end());
	}
	inline void getPositionIndexOffsets(vector<unsigned> & positionIndexOffsets)const {
		positionIndexOffsets.insert(positionIndexOffsets.end(),this->positionIndexOffsets.begin(),this->positionIndexOffsets.end());
	}

	// setters
	inline void setRecordId(unsigned id) {
		this->recordId = id;
	}
	inline void setRecordStaticScore(float staticScore) {
		this->recordStaticScore = staticScore;
	}
	inline void setRecordRuntimeScore(float runtimeScore) {
		this->recordRuntimeScore = runtimeScore;
	}
	inline void setRecordMatchingPrefixes(const vector<TrieNodePointer> & matchingPrefixes) {
		this->matchingPrefixes = matchingPrefixes;
	}
	inline void setRecordMatchEditDistances(const vector<unsigned> & editDistances) {
		this->editDistances = editDistances;
	}
	inline void setRecordMatchAttributeBitmaps(const vector<unsigned> & attributeBitmaps) {
		this->attributeBitmaps = attributeBitmaps;
	}
	inline void setPositionIndexOffsets(const vector<unsigned> & positionIndexOffsets){
		this->positionIndexOffsets = positionIndexOffsets;
	}

    unsigned getNumberOfBytes(){
    	return sizeof(recordId) +
    			sizeof(recordRuntimeScore) +
    			sizeof(recordStaticScore) +
    			sizeof(TrieNodePointer) * matchingPrefixes.size() +
    			sizeof(unsigned) * editDistances.size() +
    			sizeof(unsigned) * attributeBitmaps.size() +
    			sizeof(unsigned) * positionIndexOffsets.size();
    }

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

	PhysicalPlanRecordItemFactory(){
//		size = 0;
	}

	PhysicalPlanRecordItem * createRecordItem(){
//		if(size >= 10000){
			PhysicalPlanRecordItem  * newObj = new PhysicalPlanRecordItem();
			extraObjects.push_back(newObj);
			return newObj;
//		}else{
//			PhysicalPlanRecordItem * toReturn = &(objects[size]);
//			size ++;
//			return toReturn;
//		}
	}

	// if we get a pointer from this function, we are responsible of
	// deallocating it
	PhysicalPlanRecordItem * cloneForCache(PhysicalPlanRecordItem * oldObj){
		PhysicalPlanRecordItem  * newObj = new PhysicalPlanRecordItem();
		newObj->setRecordId(oldObj->getRecordId());
		newObj->setRecordRuntimeScore(oldObj->getRecordRuntimeScore());
		vector<TrieNodePointer> matchingPrefixes;
		oldObj->getRecordMatchingPrefixes(matchingPrefixes);
		newObj->setRecordMatchingPrefixes(matchingPrefixes);
		vector<unsigned> editDistances;
		oldObj->getRecordMatchEditDistances(editDistances);
		newObj->setRecordMatchEditDistances(editDistances);
		vector<unsigned> attributeBitmaps;
		oldObj->getRecordMatchAttributeBitmaps(attributeBitmaps);
		newObj->setRecordMatchAttributeBitmaps(attributeBitmaps);
		vector<unsigned> positionIndexOffsets;
		oldObj->getPositionIndexOffsets(positionIndexOffsets);
		newObj->setPositionIndexOffsets(positionIndexOffsets);

		return newObj;
	}

	/*
	 * This factory will take care of deallocation of these pointers.
	 */
	PhysicalPlanRecordItem * clone(PhysicalPlanRecordItem * oldObj){
		PhysicalPlanRecordItem  * newObj = createRecordItem();
		newObj->setRecordId(oldObj->getRecordId());
		newObj->setRecordRuntimeScore(oldObj->getRecordRuntimeScore());
		vector<TrieNodePointer> matchingPrefixes;
		oldObj->getRecordMatchingPrefixes(matchingPrefixes);
		newObj->setRecordMatchingPrefixes(matchingPrefixes);
		vector<unsigned> editDistances;
		oldObj->getRecordMatchEditDistances(editDistances);
		newObj->setRecordMatchEditDistances(editDistances);
		vector<unsigned> attributeBitmaps;
		oldObj->getRecordMatchAttributeBitmaps(attributeBitmaps);
		newObj->setRecordMatchAttributeBitmaps(attributeBitmaps);
		vector<unsigned> positionIndexOffsets;
		oldObj->getPositionIndexOffsets(positionIndexOffsets);
		newObj->setPositionIndexOffsets(positionIndexOffsets);

		return newObj;
	}


	void refresh(){
		vector<PhysicalPlanRecordItem *> backup;
		for(int i=0;i< extraObjects.size(); i++){
			backup.push_back(extraObjects.at(i));
		}
		extraObjects.clear();
		backups.push_back(backup);
	}

	~PhysicalPlanRecordItemFactory(){
//		cout << size + extraObjects.size() << "$";
		if(extraObjects.size() > 0){
			for(unsigned i =0 ; i< extraObjects.size() ; ++i){
				if(extraObjects.at(i) == NULL){
					ASSERT(false);
				}else{
					delete extraObjects.at(i);
				}
			}
		}
//		if(extraObjects.size() > 0){
//			cout << "extraObjectSize : " << extraObjects.size() << "\t";
//		}
		for(int b = 0; b<backups.size();++b){
//		    // start the timer for search
//		    struct timespec tstart;
//		    struct timespec tend;
//		    clock_gettime(CLOCK_REALTIME, &tstart);
//		    cout << backups.at(b).size() << "$";
			for(unsigned i =0 ; i< backups.at(b).size() ; ++i){
				if(backups.at(b).at(i) == NULL){
					ASSERT(false);
				}else{
					delete backups.at(b).at(i);
				}
			}
//		    // compute elapsed time in ms , end the timer
//		    clock_gettime(CLOCK_REALTIME, &tend);
//		    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000000
//		            + (tend.tv_nsec - tstart.tv_nsec) / 1000;
//		    cout << ts1*1.0/1000 << "\t";
		}
//		cout << endl;
	}
private:
	vector<PhysicalPlanRecordItem *> extraObjects;
	vector<vector<PhysicalPlanRecordItem *> > backups;
//	PhysicalPlanRecordItem objects[10000];
//	unsigned size;
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
	double cost;
	PhysicalPlanCost(){
		cost = 0;
	}
	PhysicalPlanCost(double c){
		cost = c;
	}
	PhysicalPlanCost(const PhysicalPlanCost & src){
		cost = src.cost;
	}
	PhysicalPlanCost operator+(const PhysicalPlanCost & rightValue){
		return PhysicalPlanCost( cost + rightValue.cost);
	}
	PhysicalPlanCost operator+(const double & rightValue){
		return PhysicalPlanCost( cost + rightValue);
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
	inline unsigned getChildrenCount() {
		return children.size();
	}
	inline PhysicalPlanOptimizationNode * getChildAt(unsigned offset) {
		if(offset >= children.size()){
			ASSERT(false);
			return NULL;
		}
		return children.at(offset);
	}
	void setChildAt(unsigned offset, PhysicalPlanOptimizationNode * child) ;
	void addChild(PhysicalPlanOptimizationNode * child) ;
	void setParent(PhysicalPlanOptimizationNode * parent);
	PhysicalPlanOptimizationNode * getParent();
	virtual ~PhysicalPlanOptimizationNode(){}
	void setExecutableNode(PhysicalPlanNode * node){
		this->executableNode = node;
	}
	inline PhysicalPlanNode * getExecutableNode(){
		return this->executableNode;
	}

	void setLogicalPlanNode(LogicalPlanNode * node){
		this->logicalPlanNode = node;
	}
	inline LogicalPlanNode * getLogicalPlanNode(){
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
	inline PhysicalPlanOptimizationNode * getPhysicalPlanOptimizationNode(){
		return this->optimizationNode;
	}

	// this function checks to see if a record (that its id can be found in parameters) is among the
	// results if this subtree. Different operators have different implementations this function.
	// When verification is performed, some information like match prefix is calculated and saved in
	// members of parameters argument, so if this function returns true, we use parameters members to
	// get that information.
	virtual bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) = 0;


	/*
	 * The implementor of this function is supposed to append a unique string to 'uniqueString'
	 * which determines the subtree of physical plan uniquely.
	 */
	void getUniqueStringForCache(bool ignoreLastLeafNode, string & uniqueString);

	/*
	 * returns a unique string which represents this operator
	 */
	virtual string toString() = 0;
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
