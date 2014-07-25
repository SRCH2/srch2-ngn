/*
 * RandomAccessVerificationGeoOperator.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: mahdi
 */

#include "PhysicalOperators.h"
#include "PhysicalOperatorsHelper.h"
#include "src/core/util/RecordSerializerUtil.h"
#include "src/core/util/RecordSerializer.h"

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
	// 1- get the forwardlist to get the location of the record from it
	bool valid = false;
	const ForwardList* forwardList = this->queryEvaluator->getForwardIndex()->getForwardList(
			parameters.forwardListDirectoryReadView,
			parameters.recordToVerify->getRecordId(),
			valid);
	if(!valid){ // this record is invalid
		return false;
	}
	// 2- find the latitude and longitude of this record
	StoredRecordBuffer buffer = forwardList->getInMemoryData();
	Schema * storedSchema = Schema::create();
	srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema, queryEvaluator->getSchema());
	srch2::util::RecordSerializer compactRecDeserializer = srch2::util::RecordSerializer(*storedSchema);

	// get the name of the attributes
	//string nameOfLatitudeAttribute = this->queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->nameOfLatitudeAttribute;
	//string nameOfLongitudeAttribute = this->queryEvaluator->getQueryEvaluatorRuntimeParametersContainer()->nameOfLongitudeAttribute;
	const string* nameOfLatitudeAttribute = this->queryEvaluator->getSchema()->getNameOfLatituteAttribute();
	const string* nameOfLongitudeAttribute = this->queryEvaluator->getSchema()->getNameOfLongitudeAttribute();
	Point point;

	unsigned idLat = storedSchema->getRefiningAttributeId(*nameOfLatitudeAttribute);
	unsigned lenOffsetLat = compactRecDeserializer.getRefiningOffset(idLat);

	point.x = *((float *)(buffer.start.get()+lenOffsetLat));

	unsigned idLong = storedSchema->getRefiningAttributeId(*nameOfLongitudeAttribute);
	unsigned lenOffsetLong = compactRecDeserializer.getRefiningOffset(idLong);
	point.y = *((float *)(buffer.start.get()+lenOffsetLong));

	// verify the record. The query region should contains this record
	if(this->queryShape->contains(point)){
		parameters.isGeo = true;
		parameters.GeoScore = parameters.ranker->computeScoreforGeo(point,*(this->queryShape));
		return true;
	}
	return false;
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
