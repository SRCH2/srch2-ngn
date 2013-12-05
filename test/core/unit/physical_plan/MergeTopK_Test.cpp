#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"
#include "PhysicalPlanTestHelper.h"
#include "operation/physical_plan/MergeTopKOperator.h"

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (3,5),(2,1),(5,0),(4,0),(1,0)
	 * List2 : (2,2),(3,1),(1,0)
	 * List3 : (2,1),(3,0),(5,0),(6,0),(7,0)
	 * List4 : (2,2),(3,1),(5,0),(4,0),(6,0),(1,0),(7,0),(8,0)
	 *
	 * Output : (3,7),(2,6)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (3,5),(2,1),(5,0),(4,0),(1,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (2,2),(3,1),(1,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(2); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (2,1),(3,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(1); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (2,2),(3,1),(5,0),(4,0),(6,0),(1,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(2); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeTopKOperator
	PhysicalOperatorFactory operatorFactory;
	MergeTopKOperator * mergeOp = operatorFactory.createMergeTopKOperator();
	MergeTopKOptimizationOperator * mergeOpOp = operatorFactory.createMergeTopKOptimizationOperator();
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

	unsigned correctResultsArray[2] = {3,2};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+2);
	ASSERT(checkResults(correctResults,operatorResults));

}


void test2(){
	/*
	 * List1 : EMPTY
	 * List2 : (2,0),(1,0),(3,0)
	 * List3 : (5,0),(2,0),(3,0),(6,0),(7,0)
	 * List4 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	 *
	 * Output : EMPTY
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : EMPTY
	vector<PhysicalPlanRecordItem *> List1;
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (2,0),(1,0),(3,0)
	vector<PhysicalPlanRecordItem *> List2;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (5,0),(2,0),(3,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeTopKOperator
	PhysicalOperatorFactory operatorFactory;
	MergeTopKOperator * mergeOp = operatorFactory.createMergeTopKOperator();
	MergeTopKOptimizationOperator * mergeOpOp = operatorFactory.createMergeTopKOptimizationOperator();
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
	 * List1 : (3,3),(4,3),(9,3),(2,0),(5,0)
	 * List2 : (3,3),(4,3),(9,3),(1,0),(2,0)
	 * List3 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	 * List4 : (4,3),(9,3),(3,2),(1,0),(2,0),(5,0),(6,0),(7,0),(8,0)
	 *
	 * Output : (9,13),(4,12),(3,11)
	 */
	PhysicalPlanRecordItemFactory recordFactory;

	// List1 : (3,3),(4,3),(9,3),(2,0),(5,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (3,3),(4,3),(9,3),(1,0),(2,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(4); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (4,3),(9,3),(3,2),(1,0),(2,0),(5,0),(6,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordFactory.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(2); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordFactory.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_UnionLowestLevelTermVirtualList);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// MergeTopKOperator
	PhysicalOperatorFactory operatorFactory;
	MergeTopKOperator * mergeOp = operatorFactory.createMergeTopKOperator();
	MergeTopKOptimizationOperator * mergeOpOp = operatorFactory.createMergeTopKOptimizationOperator();
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

	unsigned correctResultsArray[3] = {9,4,3};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+3);
	ASSERT(checkResults(correctResults,operatorResults));

}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "MergeTopK_Test: Passed\n" << endl;
}
