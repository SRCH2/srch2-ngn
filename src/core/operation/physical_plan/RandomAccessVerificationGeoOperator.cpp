/*
 * RandomAccessVerificationGeoOperator.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: mahdi
 */

#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"
#include "PhysicalOperatorsHelper.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

bool RandomAccessVerificationGeoOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = queryEvaluator;
	this->quadtree = queryEvaluator->getQuadTree();
	// get the forward list read view
	this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(this->forwardListDirectoryReadView);
	this->queryShape = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->regionShape;
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
	return verifyByRandomAccessGeoHelper(parameters, this->queryEvaluator, this->queryShape);
}

RandomAccessVerificationGeoOperator::~RandomAccessVerificationGeoOperator(){

}

RandomAccessVerificationGeoOperator::RandomAccessVerificationGeoOperator(){
	this->queryEvaluator = NULL;
	this->queryShape = NULL;
}


PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	return resultCost;
}

PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	return resultCost;
}

PhysicalPlanCost RandomAccessVerificationGeoOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
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
