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
/*
 * RandomAccessVerificationGeoOperator.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: mahdi
 */

#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"
#include "PhysicalOperatorsHelper.h"
#include "src/core/util/Logger.h"
#include "src/core/util/RecordSerializerUtil.h"
#include "src/core/util/RecordSerializer.h"

using namespace std;
using srch2::util::Logger;

namespace srch2 {
namespace instantsearch {

bool RandomAccessVerificationGeoOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = queryEvaluator;
	// get the forward list read view
	this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(this->forwardListDirectoryReadView);
	this->queryShape = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->regionShape;

	// finding the offset of the latitude and longitude attribute in the refining attributes' memory
	// we put this part in open function because we don't want to repeat it for each record
	// base on our experiments this part of the code takes more time. So it is more sufficient to use it here and save
	// the offset of latitude and longitude in the class.
	getLat_Long_Offset(this->latOffset, this->longOffset, queryEvaluator->getSchema());
    return true;
}

PhysicalPlanRecordItem * RandomAccessVerificationGeoOperator::getNext(const PhysicalPlanExecutionParameters & params){
	return NULL;
}

bool RandomAccessVerificationGeoOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = NULL;
	this->queryShape = NULL;
	return true;
}

string RandomAccessVerificationGeoOperator::toString(){
	string result = "RandomAccessVerificationGeoOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool RandomAccessVerificationGeoOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters){
	return verifyByRandomAccessGeoHelper(parameters, this->queryEvaluator, this->queryShape, this->latOffset, this->longOffset);
}

RandomAccessVerificationGeoOperator::~RandomAccessVerificationGeoOperator(){

}

RandomAccessVerificationGeoOperator::RandomAccessVerificationGeoOperator(){
	this->queryEvaluator = NULL;
	this->queryShape = NULL;
}


PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	// we only need to check if the query region contains the record's point or not
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

void RandomAccessVerificationGeoOptimizationOperator::getOutputProperties(IteratorProperties & prop){

}

void RandomAccessVerificationGeoOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){

}

PhysicalPlanNodeType RandomAccessVerificationGeoOptimizationOperator::getType(){
	return PhysicalPlanNode_RandomAccessGeo;
}

bool RandomAccessVerificationGeoOptimizationOperator::validateChildren(){
	// this operator cannot have any children
	if(getChildrenCount() > 0){
		return false;
	}
	return true;
}



}
}
