/*
 * GeoSimpleScanOperator.cpp
 *
 *  Created on: Jul 10, 2014
 *      Author: mahdi
 */

#include "GeoSimpleScanOperator.h"

using namespace std;

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

	//TODO: After adding the new logical node code get the shape of the query region from it. Then remove this code.
	Point point; // TODO remove this line
	point.x = 0; // TODO remove this line
	point.y = 0; // TODO remove this line
	this->queryShape = new Circle(point,10); // TODO get the shape from the LogicalPlanNode

	this->quadtree->rangeQuery(this->geoElements,*this->queryShape);
	// set the offsets to use them in getNext
	this->vectorOffset = 0;
	this->cursorOnVectorOfGeoElements = 0;

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
		if(this->queryShape->contains(element->point)){
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
	// record id
	newItem->setRecordId(element->forwardListID);
	// runtime score
	//TODO check if the type of the params.ranker is set to spatialRanker for this operation or not
	// ASSERT(typeof(*params.ranker) == typeof(SpatialRanker));
	newItem->setRecordRuntimeScore(element->getScore((SpatialRanker*)params.ranker,*this->queryShape));

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
	// First get the forwardlist to get the location of the record from it
	bool valid = false;
	const ForwardList* forwardList = this->queryEvaluator->getForwardIndex()->getForwardList(
			parameters.forwardListDirectoryReadView,
			parameters.recordToVerify->getRecordId(),
			valid);
	//TODO Here for now I assume that I can get the location information of the record from the forwardlist
	Point point; // = forwardList->getLocation();

	// verify the record. The query region should contains this record
	return this->queryShape->contains(point);
}

}
}


