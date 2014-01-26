#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"

using namespace srch2::instantsearch;


class TestLowLevelOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
		cursor = 0;
		return true;
	}
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) {
		if(cursor < records.size()){
			return records[cursor++];
		}else{
			return NULL;
		}
	}
	bool close(PhysicalPlanExecutionParameters & params){
		cursor = 0;
		return true;
	}

	string toString(){
		return "TestLowLevelOperator" ;
	}

	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
		for(unsigned i=0; i<records.size(); i++){
			if(records.at(i)->getRecordId() == parameters.recordToVerify->getRecordId()){
				parameters.runTimeTermRecordScore = records.at(i)->getRecordRuntimeScore();
				return true;
			}
		}

		return false;
	}
	~TestLowLevelOperator(){};
	TestLowLevelOperator(vector<PhysicalPlanRecordItem *> records){
		cursor = 0;
		this->records = records;
	}

private :
	unsigned cursor ;
	vector<PhysicalPlanRecordItem *> records;
};

class TestLowLevelOptimizationOperator : public PhysicalPlanOptimizationNode {
	friend class PhysicalOperatorFactory;
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
		return PhysicalPlanCost(1);
	}
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
		return PhysicalPlanCost(1);
	}
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) {
		return PhysicalPlanCost(1);
	}
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
		return PhysicalPlanCost(1);
	}
	void getOutputProperties(IteratorProperties & prop){

	}
	void getRequiredInputProperties(IteratorProperties & prop){

	}
	PhysicalPlanNodeType getType() {
		return type;
	}
	bool validateChildren(){
		return true;
	}
	TestLowLevelOptimizationOperator(PhysicalPlanNodeType type){
		this->type = type;
	}
	PhysicalPlanNodeType type;
};

bool checkResults(vector<unsigned> & correctResults, vector<unsigned> & operatorResults){
	if(correctResults.size() != operatorResults.size()){
		return false;
	}
	for(unsigned i = 0 ; i < correctResults.size() ; i++){
		if(correctResults[i] != operatorResults[i]){
			return false;
		}
	}
	return true;
}
