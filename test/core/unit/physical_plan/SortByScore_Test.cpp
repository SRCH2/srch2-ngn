#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"
#include "PhysicalPlanTestHelper.h"

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	 *
	 * Output : (3,8),(8,7),(7,6),(1,5),(2,4),(6,3),(5,2),(4,1)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(8); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(2); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(4); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(6); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(7); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);


	// SortByScore
	PhysicalOperatorFactory operatorFactory;
	SortByScoreOperator * sortOp = operatorFactory.createSortByScoreOperator();
	SortByScoreOptimizationOperator * sortOpOp = operatorFactory.createSortByScoreOptimizationOperator();
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

	unsigned correctResultsArray[8] = {3,8,7,1,2,6,5,4};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+8);
	ASSERT(checkResults(correctResults,operatorResults));

}


void test2(){
	/*
	 * List1 : EMPTY
	 *
	 * Output : EMPTY
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : EMPTY
	vector<PhysicalPlanRecordItem *> List1;
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// SortByScore
	PhysicalOperatorFactory operatorFactory;
	SortByScoreOperator * mergeOp = operatorFactory.createSortByScoreOperator();
	SortByScoreOptimizationOperator * mergeOpOp = operatorFactory.createSortByScoreOptimizationOperator();
	mergeOp->setPhysicalPlanOptimizationNode(mergeOpOp);
	mergeOpOp->setExecutableNode(mergeOp);

	mergeOpOp->addChild(&list1OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	mergeOp->open(NULL, params);
	vector<unsigned> operatorResults;
	while(true){
		PhysicalPlanRecordItem * record = mergeOp->getNext(params);
		if(record == NULL){
			break;
		}
		operatorResults.push_back(record->getRecordId());
	}
	mergeOp->close(params);

	vector<unsigned> correctResults; // empty
	ASSERT(checkResults(correctResults,operatorResults));

}

void test3(){
	/*
	 * List1 : (3,1),(5,1),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	 *
	 * Output : (8,7),(7,6),(1,5),(2,4),(6,3),(3,1),(4,1),(5,1)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(4); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(6); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(7); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);


	// SortByScore
	PhysicalOperatorFactory operatorFactory;
	SortByScoreOperator * sortOp = operatorFactory.createSortByScoreOperator();
	SortByScoreOptimizationOperator * sortOpOp = operatorFactory.createSortByScoreOptimizationOperator();
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

	unsigned correctResultsArray[8] = {8,7,1,2,6,3,4,5};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+8);
	ASSERT(checkResults(correctResults,operatorResults));

}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "SortByScore_Test: Passed\n" << endl;
}
