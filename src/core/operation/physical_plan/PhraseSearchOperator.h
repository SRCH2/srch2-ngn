
#ifndef __PHYSICALPLAN_PHRASESEARCHOPERATOR_H__
#define __PHYSICALPLAN_PHRASESEARCHOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"
#include "operation/PhraseSearcher.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class PhraseSearchOperator : public PhysicalPlanNode {
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	~PhraseSearchOperator();
	PhraseSearchOperator(const PhraseInfo& phraseSearchInfo) ;

private:
	bool phraseErr;
	PhraseInfo phraseSearchInfo;
	QueryEvaluatorInternal * queryEvaluatorInternal;
	PhraseSearcher *phraseSearcher;
	// match phrase on attributes. do OR or AND logic depending upon the 32 bit of attributeBitMap
	bool matchPhrase(const ForwardList* forwardListPtr, const PhraseInfo& phraseInfo, vector<unsigned> &listOfSlops);
	PhysicalPlanRecordItem * getNextCandidateRecord(const PhysicalPlanExecutionParameters & params);

};

class PhraseSearchOptimizationOperator : public PhysicalPlanOptimizationNode {
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};


}
}


#endif // __PHYSICALPLAN_PHRASESEARCHOPERATOR_H__
