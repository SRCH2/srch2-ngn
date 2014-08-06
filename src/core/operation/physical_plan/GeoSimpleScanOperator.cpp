/*
 * GeoSimpleScanOperator.cpp
 *
 *  Created on: Jul 10, 2014
 *      Author: mahdi
 */

#include "GeoSimpleScanOperator.h"
#include "PhysicalOperatorsHelper.h"
#include "src/core/util/Logger.h"
#include "src/core/util/RecordSerializerUtil.h"
#include "src/core/util/RecordSerializer.h"

using namespace std;
using srch2::util::Logger;

namespace srch2{
namespace instantsearch{

GeoSimpleScanOperator::GeoSimpleScanOperator(){
	this->queryEvaluator = NULL;
	this->queryShape = NULL;
}

GeoSimpleScanOperator::~GeoSimpleScanOperator(){

}

bool GeoSimpleScanOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// first save the pointer to QueryEvaluator
	this->queryEvaluator = queryEvaluator;
	//TODO: after adding concurrency this line should be like: this->queryEvaluator->getQuadTree()->getQuadTree_ReadView(quadtree);
	this->quadtree = this->queryEvaluator->getQuadTree();
	// get the forward list read view
	this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(this->forwardListDirectoryReadView);
	// get the query shape
	this->queryShape = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->regionShape;
	// get quadTreeNodeSet which contains all the subtrees in quadtree which have the answers
	vector<QuadTreeNode*>* quadTreeNodeSet = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getQuadTreeNodeSetForEstimation();

	// put the vector of element in each node in geoElements
	// rangeQuery will be called recursively to find all the leaf nodes.
	for( unsigned i = 0 ; i < quadTreeNodeSet->size() ; i++){
		quadTreeNodeSet->at(i)->rangeQuery(this->geoElements, *this->queryShape);
	}

	int count = 0;
	for ( int i = 0 ; i < this->geoElements.size() ; i++){
		vector<GeoElement*> tmp = *(this->geoElements[i]);
		for ( int j = 0; j < tmp.size() ; j++){
			if(this->queryShape->contain(tmp[j]->point)){
				count++;
				break;
			}
		}
	}
	// set the offsets to use them in getNext
	this->vectorOffset = 0;
	this->cursorOnVectorOfGeoElements = 0;

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

	return true;
}

PhysicalPlanRecordItem* GeoSimpleScanOperator::getNext(const PhysicalPlanExecutionParameters & params){
	// check if we reach to the end of the vector
	if(this->vectorOffset >= this->geoElements.size())
		return NULL;

	ASSERT(this->cursorOnVectorOfGeoElements < this->geoElements[this->vectorOffset]->size());

	// Iterate through the records to find the first valid record
	GeoElement* element;
	vector<GeoElement*> * leafElements;
	bool foundValidHit = false;
	while(this->vectorOffset < this->geoElements.size()){
		leafElements = this->geoElements[this->vectorOffset];
		element = (*leafElements)[this->cursorOnVectorOfGeoElements];
		// check the record and return it if it's valid.
		bool valid = false;
		const ForwardList* forwardList = this->queryEvaluator->getForwardIndex()->getForwardList(
				 this->forwardListDirectoryReadView,
				 element->forwardListID,
				 valid);
		if(valid && this->queryShape->contain(element->point)){
			foundValidHit = true;
			break;
		}
		// Increase the offsets
		this->cursorOnVectorOfGeoElements++;
		if(this->cursorOnVectorOfGeoElements >= leafElements->size()){
			this->cursorOnVectorOfGeoElements = 0;
			this->vectorOffset++;
		}
	}

	if(!foundValidHit){
		return NULL;
	}

	// Create the item to return.
	PhysicalPlanRecordItem* newItem = this->queryEvaluator->getPhysicalPlanRecordItemPool()->createRecordItem();
	newItem->setIsGeo(true); // this Item is for a geo element
	// record id
	newItem->setRecordId(element->forwardListID);
	// runtime score
	newItem->setRecordRuntimeScore(element->getScore(*this->queryShape));

	// increase the offsets for next use of getNext
	this->cursorOnVectorOfGeoElements++;
	if(this->cursorOnVectorOfGeoElements >= leafElements->size()){
		this->cursorOnVectorOfGeoElements = 0;
		this->vectorOffset++;
	}

	return newItem;
}

bool GeoSimpleScanOperator::close(PhysicalPlanExecutionParameters & params){
	this->cursorOnVectorOfGeoElements = 0;
	this->vectorOffset = 0;
	this->queryEvaluator = NULL;
	this->quadtree = NULL;
	this->geoElements.clear();
	return true;
}

string GeoSimpleScanOperator::toString(){
	string result = "GeoSimpleScanOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool GeoSimpleScanOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters){
	return verifyByRandomAccessGeoHelper(parameters, this->queryEvaluator, this->queryShape, this->latOffset, this->longOffset);
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost GeoSimpleScanOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	// cost of going over leaf nodes to return their vector of elements
	PhysicalPlanCost resultCost;
	resultCost.cost = this->getLogicalPlanNode()->stats->estimatedNumberOfLeafNodes;
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost GeoSimpleScanOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost GeoSimpleScanOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

PhysicalPlanCost GeoSimpleScanOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	// we only need to check if the query region contains the record's point or not
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

void GeoSimpleScanOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// no output property
}

void GeoSimpleScanOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}

PhysicalPlanNodeType GeoSimpleScanOptimizationOperator::getType(){
	return PhysicalPlanNode_GeoSimpleScan;
}

bool GeoSimpleScanOptimizationOperator::validateChildren(){
	// this operator cannot have any children
	if(getChildrenCount() > 0){
		return false;
	}
	return true;
}

}
}


