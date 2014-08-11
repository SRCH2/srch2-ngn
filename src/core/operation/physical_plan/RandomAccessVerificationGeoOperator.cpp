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
	Schema * storedSchema = Schema::create();
	srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema, queryEvaluator->getSchema());
	srch2::util::RecordSerializer compactRecDeserializer = srch2::util::RecordSerializer(*storedSchema);

	// get the name of the attributes
	const string* nameOfLatitudeAttribute = queryEvaluator->getSchema()->getNameOfLatituteAttribute();
	const string* nameOfLongitudeAttribute = queryEvaluator->getSchema()->getNameOfLongitudeAttribute();

	unsigned idLat = storedSchema->getRefiningAttributeId(*nameOfLatitudeAttribute);
	this->latOffset = compactRecDeserializer.getRefiningOffset(idLat);

	unsigned idLong = storedSchema->getRefiningAttributeId(*nameOfLongitudeAttribute);
	this->longOffset = compactRecDeserializer.getRefiningOffset(idLong);
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
