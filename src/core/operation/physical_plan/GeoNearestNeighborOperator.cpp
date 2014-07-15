/*
 * GeoNearestNeighborOperator.cpp
 *
 *  Created on: Jul 11, 2014
 *      Author: mahdi
 */
#include "GeoNearestNeighborOperator.h"

namespace srch2 {
namespace instantsearch {

GeoNearestNeighborOperator::GeoNearestNeighborOperator(){
	this->queryEvaluator = NULL;
	this->queryShape = NULL;
}

GeoNearestNeighborOperator::~GeoNearestNeighborOperator(){
	for(unsigned i = 0 ; i < this->heapItems.size() ; i++){
		delete this->heapItems[i];
	}
	this->heapItems.clear();
}

bool GeoNearestNeighborOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// saving the pointer to QueryEvaluator
	this->queryEvaluator = queryEvaluator;
	// saving the pointer to the quadtree
	this->quadtree = queryEvaluator->getQuadTree();
	// finding the query region

	//TODO: After adding the new logical node code get the shape of the query region from it. Then remove this code.
	Point point; // TODO remove this line
	point.x = 0; // TODO remove this line
	point.y = 0; // TODO remove this line
	this->queryShape = new Circle(point,10); // TODO get the shape from the LogicalPlanNode

	//put the root of the tree in the heap to use in getNext
	if(this->quadtree->getRoot() != NULL){
		this->heapItems.push_back(new GeoNearestNeighborOperatorHeapItem(this->quadtree->getRoot(),this->queryShape));
		make_heap(this->heapItems.begin(),this->heapItems.end(),GeoNearestNeighborOperator::GeoNearestNeighborOperatorHeapItemCmp());
	}

	return true;
}

PhysicalPlanRecordItem* GeoNearestNeighborOperator::getNext(const PhysicalPlanExecutionParameters & params){
	GeoNearestNeighborOperatorHeapItem* heapItem;
	// Iterate on heap until find a geoElement on top of the heap
	while(this->heapItems.size() > 0){
		// pop the first item of the heap
		heapItem = this->heapItems.front();
		pop_heap(heapItems.begin(),heapItems.end(), GeoNearestNeighborOperator::GeoNearestNeighborOperatorHeapItemCmp());
		heapItems.pop_back();

		if(heapItem->isQuadTreeNode){ // the top of the heap is a quadtree node.
			if(heapItem->quadtreeNode->getIsLeaf()){ // this node is leaf. So we should insert all its elements to the heap.
				vector<GeoElement*>* elements = heapItem->quadtreeNode->getElements();
				for( unsigned i = 0 ; i < elements->size() ; i++ ){
					if(queryShape->contains(elements->at(i)->point)){
						heapItems.push_back(new GeoNearestNeighborOperatorHeapItem(elements->at(i), queryShape));
						// correct the heap
						push_heap(heapItems.begin(),
								  heapItems.end(),
								  GeoNearestNeighborOperator::GeoNearestNeighborOperatorHeapItemCmp());
					}
				}
			}else{ // this node is an internal node. So we should insert all its children to the heap.
				vector<QuadTreeNode*>* children = heapItem->quadtreeNode->getChildren();
				for( unsigned i = 0 ; i < children->size() ; i++ ){
					if(children->at(i) != NULL){
						heapItems.push_back(new GeoNearestNeighborOperatorHeapItem(children->at(i), queryShape));
						// correct the heap
						push_heap(heapItems.begin(),
								  heapItems.end(),
								  GeoNearestNeighborOperator::GeoNearestNeighborOperatorHeapItemCmp());
					}
				}
			}
		}else{ // the top of the heap is a geoElement. So we can return it as next nearest
			PhysicalPlanRecordItem* newItem = this->queryEvaluator->getPhysicalPlanRecordItemPool()->createRecordItem();
			// record id
			newItem->setRecordId(heapItem->geoElement->forwardListID);
			// runtime score
			//TODO check if the type of the params.ranker is set to spatialRanker for this operation or not
			// ASSERT(typeof(*params.ranker) == typeof(SpatialRanker));
			newItem->setRecordRuntimeScore(heapItem->geoElement->getScore((SpatialRanker*)params.ranker,*this->queryShape));
			delete heapItem;
			return newItem;
		}
		delete heapItem;
	}
	return NULL;
}

bool GeoNearestNeighborOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = NULL;
	for( unsigned i = 0 ; i < this->heapItems.size() ; i++ )
		delete this->heapItems[i];
	this->heapItems.clear();
	this->queryShape = NULL;
	return true;
}

bool GeoNearestNeighborOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters){
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

string GeoNearestNeighborOperator::toString(){
	string result = "GeoNearestNeighborOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

}
}



