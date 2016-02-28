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
