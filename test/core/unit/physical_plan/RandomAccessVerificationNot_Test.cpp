/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "operation/physical_plan/PhysicalPlan.h"
#include "operation/physical_plan/PhysicalOperators.h"
#include "PhysicalPlanTestHelper.h"
#include "operation/physical_plan/MergeTopKOperator.h"

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (2,2),(3,1),(5,0),(4,0),(6,0),(1,0),(7,0),(8,0)
	 *
	 * Output : EMPTY
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (2,2),(3,1),(5,0),(4,0),(6,0),(1,0),(7,0),(8,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(2); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(8); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);


	// RandomAccessVerificationNot Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationNotOperator * verificationOp = operatorFactory.createRandomAccessVerificationNotOperator();
	RandomAccessVerificationNotOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationNotOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	PhysicalPlanRandomAccessVerificationParameters verificationParams(params.ranker,forwardListDirectoryReadView);
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

	vector<unsigned> correctResults; // EMPTY
	ASSERT(checkResults(correctResults,operatorResults));

}


void test2(){
	/*
	 * List1 : (2,0),(1,0),(3,0)
	 *
	 * Output : (4,0),(5,0),(6,0),(7,0),(8,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (2,0),(1,0),(3,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// RandomAccessVerificationNot Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationNotOperator * verificationOp = operatorFactory.createRandomAccessVerificationNotOperator();
	RandomAccessVerificationNotOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationNotOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);

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

	unsigned correctResultsArray[5] = {4,5,6,7,8};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+5);
	ASSERT(checkResults(correctResults,operatorResults));

}

void test3(){
	/*
	 * List1 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	 *
	 * Output : (2,0),(8,0)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (9,4),(3,3),(4,3),(1,0),(5,0),(6,0),(7,0)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(9); record->setRecordRuntimeScore(4); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(0); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(0); List1.push_back(record);
	TestLowLevelOperator list1Op(List1);
	TestLowLevelOptimizationOperator list1OpOp(PhysicalPlanNode_RandomAccessTerm);
	list1Op.setPhysicalPlanOptimizationNode(&list1OpOp);
	list1OpOp.setExecutableNode(&list1Op);

	// RandomAccessVerificationNot Operator
	PhysicalOperatorFactory operatorFactory;
	RandomAccessVerificationNotOperator * verificationOp = operatorFactory.createRandomAccessVerificationNotOperator();
	RandomAccessVerificationNotOptimizationOperator * verificationOpOp = operatorFactory.createRandomAccessVerificationNotOptimizationOperator();
	verificationOp->setPhysicalPlanOptimizationNode(verificationOpOp);
	verificationOpOp->setExecutableNode(verificationOp);

	verificationOpOp->addChild(&list1OpOp);

	PhysicalPlanExecutionParameters params(10,true,0.5,SearchTypeTopKQuery);
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	PhysicalPlanRandomAccessVerificationParameters verificationParams(params.ranker,forwardListDirectoryReadView);
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

	unsigned correctResultsArray[2] = {2,8};
	vector<unsigned> correctResults(correctResultsArray , correctResultsArray+2);
	ASSERT(checkResults(correctResults,operatorResults));

	recordFactory.closeRecordItemPool(poolHandle);
}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "RandomAccessVerificationNot_Test: Passed\n" << endl;
}
