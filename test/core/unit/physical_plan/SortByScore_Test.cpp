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

using namespace srch2::instantsearch;

void test1(){
	/*
	 * List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	 *
	 * Output : (3,8),(8,7),(7,6),(1,5),(2,4),(6,3),(5,2),(4,1)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(8); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(2); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(4); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(6); List1.push_back(record);
	record = recordPool.createRecordItem();
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

	recordFactory.closeRecordItemPool(poolHandle);
}

void test3(){
	/*
	 * List1 : (3,1),(5,1),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	 *
	 * Output : (8,7),(7,6),(1,5),(2,4),(6,3),(3,1),(4,1),(5,1)
	 */
	PhysicalPlanRecordItemFactory recordFactory;
	unsigned poolHandle = recordFactory.openRecordItemPool();
	PhysicalPlanRecordItemPool & recordPool = *(recordFactory.getRecordItemPool(poolHandle));

	// List1 : (3,8),(5,2),(4,1),(6,3),(1,5),(2,4),(7,6),(8,7)
	vector<PhysicalPlanRecordItem *> List1;
	PhysicalPlanRecordItem * record = recordPool.createRecordItem();
	record->setRecordId(3); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(5); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(4); record->setRecordRuntimeScore(1); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(6); record->setRecordRuntimeScore(3); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(1); record->setRecordRuntimeScore(5); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(2); record->setRecordRuntimeScore(4); List1.push_back(record);
	record = recordPool.createRecordItem();
	record->setRecordId(7); record->setRecordRuntimeScore(6); List1.push_back(record);
	record = recordPool.createRecordItem();
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

	recordFactory.closeRecordItemPool(poolHandle);
}

int main(int argc, char *argv[]) {
	test1();
	test2();
	test3();
	cout << "SortByScore_Test: Passed\n" << endl;
}
