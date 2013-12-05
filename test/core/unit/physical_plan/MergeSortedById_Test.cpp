#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"
#include "PhysicalPlanTestHelper.h"

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (1,0),(2,0),(3,0),(4,0),(5,0)
	 * List2 : (1,0),(2,0),(3,0),(4,0)
	 * List3 : (1,0),(3,0),(4.0),(5,0),(6,0),(7,0)
	 * List4 : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
	 *
	 * Output : (1,0),(2,0),(4,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (1,0),(2,0),(3,0),(4,0),(5,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_SortById);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (1,0),(2,0),(3,0),(4,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_SortById);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (1,0),(3,0),(4.0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_SortById);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_SortById);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeSortedByIdOperator
	PhysicalOperatorFactory operatorFactory;
	MergeSortedByIDOperator * mergeOp = operatorFactory.createMergeSortedByIDOperator();
	MergeSortedByIDOptimizationOperator * mergeOpOp = operatorFactory.createMergeSortedByIDOptimizationOperator();
	mergeOp->setPhysicalPlanOptimizationNode(mergeOpOp);
	mergeOpOp->setExecutableNode(mergeOp);

	mergeOpOp->addChild(&list1OpOp);
	mergeOpOp->addChild(&list2OpOp);
	mergeOpOp->addChild(&list3OpOp);
	mergeOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5);
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

	unsigned correctResultsArray[3] = {1,3,4};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+3);
	ASSERT(checkResults(correctResults,operatorResults));

}


void test2(){
	/*
	 * List1 : EMPTY
	 * List2 : (1,0),(2,0),(3,0)
	 * List3 : (2,0),(3,0),(5,0),(6,0),(7,0)
	 * List4 : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
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

	// List2 : (1,0),(2,0),(3,0)
	vector<PhysicalPlanRecordItem *> List2;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_UnionLowestLevelSimpleScanOperator);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (2,0),(3,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_RandomAccessTerm);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_RandomAccessTerm);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeSortedByIdOperator
	PhysicalOperatorFactory operatorFactory;
	MergeSortedByIDOperator * mergeOp = operatorFactory.createMergeSortedByIDOperator();
	MergeSortedByIDOptimizationOperator * mergeOpOp = operatorFactory.createMergeSortedByIDOptimizationOperator();
	mergeOp->setPhysicalPlanOptimizationNode(mergeOpOp);
	mergeOpOp->setExecutableNode(mergeOp);

	mergeOpOp->addChild(&list1OpOp);
	mergeOpOp->addChild(&list2OpOp);
	mergeOpOp->addChild(&list3OpOp);
	mergeOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5);
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
	 * List1 : (2,0),(3,0),(4,0),(5,0),(9,0)
	 * List2 : (1,0),(2,0),(3,0),(4,0),(9,0)
	 * List3 : (1,0),(3,0),(4.0),(5,0),(6,0),(7,0),(9,0)
	 * List4 : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0),(9,0)
	 *
	 * Output : (3,0),(4,0),(9,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (2,0),(3,0),(4,0),(5,0),(9,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_SortById);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (1,0),(2,0),(3,0),(4,0),(9,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_SortById);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (3,0),(4.0),(5,0),(6,0),(7,0),(9,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_SortById);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0),(9,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_SortById);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeSortedByIdOperator
	PhysicalOperatorFactory operatorFactory;
	MergeSortedByIDOperator * mergeOp = operatorFactory.createMergeSortedByIDOperator();
	MergeSortedByIDOptimizationOperator * mergeOpOp = operatorFactory.createMergeSortedByIDOptimizationOperator();
	mergeOp->setPhysicalPlanOptimizationNode(mergeOpOp);
	mergeOpOp->setExecutableNode(mergeOp);

	mergeOpOp->addChild(&list1OpOp);
	mergeOpOp->addChild(&list2OpOp);
	mergeOpOp->addChild(&list3OpOp);
	mergeOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5);
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

	unsigned correctResultsArray[3] = {3,4,9};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+3);
	ASSERT(checkResults(correctResults,operatorResults));

}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "MergeSortedById_Test: Passed\n" << endl;
}
