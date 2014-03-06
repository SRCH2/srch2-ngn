#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"
#include "PhysicalPlanTestHelper.h"

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	 *
	 * Output : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);


	// SortById
	PhysicalOperatorFactory operatorFactory;
	SortByIdOperator * sortOp = operatorFactory.createSortByIdOperator();
	SortByIdOptimizationOperator * sortOpOp = operatorFactory.createSortByIdOptimizationOperator();
	sortOp->setPhysicalPlanOptimizationNode(sortOpOp);
	sortOpOp->setExecutableNode(sortOp);

	sortOpOp->addChild(&list1OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	sortOp->open(NULL, params);
	vector<unsigned> operatorResults;
	while(true){
		PhysicalPlanRecordItem * record = sortOp->getNext(params);
		if(record == NULL){
			break;
		}
		operatorResults.push_back(record->getRecordId());
	}
	sortOp->close(params);

	unsigned correctResultsArray[8] = {1,2,3,4,5,6,7,8};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+8);
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}


void test2(){
	/*
	 * List1 : EMPTY
	 *
	 * Output : EMPTY
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : EMPTY
	vector<PhysicalPlanRecordItem *> List1;
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// SortById
	PhysicalOperatorFactory operatorFactory;
	SortByIdOperator * sortOp = operatorFactory.createSortByIdOperator();
	SortByIdOptimizationOperator * sortOpOp = operatorFactory.createSortByIdOptimizationOperator();
	sortOp->setPhysicalPlanOptimizationNode(sortOpOp);
	sortOpOp->setExecutableNode(sortOp);

	sortOpOp->addChild(&list1OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	sortOp->open(NULL, params);
	vector<unsigned> operatorResults;
	while(true){
		PhysicalPlanRecordItem * record = sortOp->getNext(params);
		if(record == NULL){
			break;
		}
		operatorResults.push_back(record->getRecordId());
	}
	sortOp->close(params);

	vector<unsigned> correctResults; // empty
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}

int main(int argc, char *argv[]) {
	test1();
	test2();
	cout << "SortById_Test: Passed\n" << endl;
}
