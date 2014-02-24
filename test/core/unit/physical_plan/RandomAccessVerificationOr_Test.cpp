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
	 * Output : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (3,5),(2,1),(5,0),(4,0),(1,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (2,2),(3,1),(1,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(2); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_RandomAccessTerm);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (2,1),(3,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(1); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_RandomAccessTerm);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (2,2),(3,1),(5,0),(4,0),(6,0),(1,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(2); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_RandomAccessTerm);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// RandomAccessVerificationOr Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationOrOperator * verificationOp = operatorFactory.createRandomAccessVerificationOrOperator();
	RandomAccessVerificationOrOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationOrOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);
	verificationOpOp->addChild(&list2OpOp);
	verificationOpOp->addChild(&list3OpOp);
	verificationOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	PhysicalPlanRandomAccessVerificationParameters verificationParams(params.ranker, forwardListDirectoryReadView);
	verificationOp->open(NULL, params);
	vector<unsigned> operatorResults;
	for(unsigned i = 1 ; i <= 8 ; ++i){
		PhysicalPlanRecordItem * record = recordPool.createRecordItem();
		record->setRecordId(i);
		verificationParams.recordToVerify = record;
		if(verificationOp->verifyByRandomAccess(verificationParams) == true){
			operatorResults.push_back(record->getRecordId());
		}
	}
	verificationOp->close(params);

	unsigned correctResultsArray[8] = {1,2,3,4,5,6,7,8};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+8);
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}


void test2(){
	/*
	 * List1 : EMPTY
	 * List2 : (2,0),(1,0),(3,0)
	 * List3 : (5,0),(2,0),(3,0),(6,0),(7,0)
	 * List4 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	 *
	 * Output : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0)
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

	// List2 : (2,0),(1,0),(3,0)
	vector<PhysicalPlanRecordItem *> List2;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_RandomAccessTerm);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (5,0),(2,0),(3,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_RandomAccessTerm);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (3,0),(5,0),(4,0),(6,0),(1,0),(2,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_RandomAccessTerm);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// RandomAccessVerificationOr Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationOrOperator * verificationOp = operatorFactory.createRandomAccessVerificationOrOperator();
	RandomAccessVerificationOrOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationOrOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);
	verificationOpOp->addChild(&list2OpOp);
	verificationOpOp->addChild(&list3OpOp);
	verificationOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	PhysicalPlanRandomAccessVerificationParameters verificationParams(params.ranker, forwardListDirectoryReadView);
	verificationOp->open(NULL, params);
	vector<unsigned> operatorResults;
	for(unsigned i = 1 ; i <= 8 ; ++i){
		PhysicalPlanRecordItem * record = recordPool.createRecordItem();
		record->setRecordId(i);
		verificationParams.recordToVerify = record;
		if(verificationOp->verifyByRandomAccess(verificationParams) == true){
			operatorResults.push_back(record->getRecordId());
		}
	}
	verificationOp->close(params);

	unsigned correctResultsArray[8] = {1,2,3,4,5,6,7,8};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+8);
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}

void test3(){
	/*
	 * List1 : (3,3),(4,3),(9,3),(2,0),(5,0)
	 * List2 : (3,3),(4,3),(9,3),(1,0),(2,0)
	 * List3 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	 * List4 : (4,3),(9,3),(3,2),(1,0),(2,0),(5,0),(6,0),(7,0),(8,0)
	 *
	 * Output : (1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0),(8,0),(9,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (3,3),(4,3),(9,3),(2,0),(5,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// List2 : (3,3),(4,3),(9,3),(1,0),(2,0)
	vector<PhysicalPlanRecordItem *> List2;
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List2.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List2.push_back(record);
	TestLowLevelOperator list2Op(List2);
	TestLowLevelOptimizationOperator list2OpOp(PhysicalPlanNode_RandomAccessTerm);
	list2Op.setPhysicalPlanOptimizationNode(&list2OpOp);
	list2OpOp.setExecutableNode(&list2Op);

	// List3 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List3;
	record = recordPool.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(4); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List3.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List3.push_back(record);
	TestLowLevelOperator list3Op(List3);
	TestLowLevelOptimizationOperator list3OpOp(PhysicalPlanNode_RandomAccessTerm);
	list3Op.setPhysicalPlanOptimizationNode(&list3OpOp);
	list3OpOp.setExecutableNode(&list3Op);

	// List4 : (4,3),(9,3),(3,2),(1,0),(2,0),(5,0),(6,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List4;
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(3); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(2); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List4.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List4.push_back(record);
	TestLowLevelOperator list4Op(List4);
	TestLowLevelOptimizationOperator list4OpOp(PhysicalPlanNode_RandomAccessTerm);
	list4Op.setPhysicalPlanOptimizationNode(&list4OpOp);
	list4OpOp.setExecutableNode(&list4Op);


	// RandomAccessVerificationOr Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationOrOperator * verificationOp = operatorFactory.createRandomAccessVerificationOrOperator();
	RandomAccessVerificationOrOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationOrOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);
	verificationOpOp->addChild(&list2OpOp);
	verificationOpOp->addChild(&list3OpOp);
	verificationOpOp->addChild(&list4OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	PhysicalPlanRandomAccessVerificationParameters verificationParams(params.ranker, forwardListDirectoryReadView);
	verificationOp->open(NULL, params);
	vector<unsigned> operatorResults;
	for(unsigned i = 1 ; i <= 9 ; ++i){
		PhysicalPlanRecordItem * record = recordPool.createRecordItem();
		record->setRecordId(i);
		verificationParams.recordToVerify = record;
		if(verificationOp->verifyByRandomAccess(verificationParams) == true){
			operatorResults.push_back(record->getRecordId());
		}
	}
	verificationOp->close(params);

	unsigned correctResultsArray[9] = {1,2,3,4,5,6,7,8,9};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+9);
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "RandomAccessVerificationOr_Test: Passed\n" << endl;
}
