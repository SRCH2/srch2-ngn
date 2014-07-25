/*
 * GeoNearestNeighborOperator.cpp
 *
 *  Created on: Jul 11, 2014
 *      Author: mahdi
 */
#include "GeoNearestNeighborOperator.h"
#include "src/core/util/RecordSerializerUtil.h"
#include "src/core/util/RecordSerializer.h"

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
	this->queryEvaluator = queryEvaluator;
	this->quadtree = queryEvaluator->getQuadTree();
	// get the forward list read view
	this->queryEvaluator->getForwardIndex()->getForwardListDirectory_ReadView(this->forwardListDirectoryReadView);
	// finding the query region
	this->queryShape = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->regionShape;
	// get quadTreeNodeSet which contains all the subtrees in quadtree which have the answers
	vector<QuadTreeNode*>* quadTreeNodeSet = this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->stats->getQuadTreeNodeSetForEstimation();
	// put all the QuadTree nodes from quadTreeNodeSet into the heap vector

	for( unsigned i = 0 ; i < quadTreeNodeSet->size() ; i++ ){
		this->heapItems.push_back(new GeoNearestNeighborOperatorHeapItem(quadTreeNodeSet->at(i),this->queryShape));
	}
	// make the heap
	make_heap(this->heapItems.begin(),this->heapItems.end(),GeoNearestNeighborOperator::GeoNearestNeighborOperatorHeapItemCmp());

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
		}else{ // the top of the heap is a geoElement. So we can return it if it is in the query region
			// check the record and return it if it's valid.
			bool valid = false;
			const ForwardList* forwardList = this->queryEvaluator->getForwardIndex()->getForwardList(
					 this->forwardListDirectoryReadView,
					 heapItem->geoElement->forwardListID,
					 valid);
			if(valid & this->queryShape->contains(heapItem->geoElement->point)){
				PhysicalPlanRecordItem* newItem = this->queryEvaluator->getPhysicalPlanRecordItemPool()->createRecordItem();
				newItem->setIsGeo(true); // this Item is for a geo element
				// record id
				newItem->setRecordId(heapItem->geoElement->forwardListID);
				// runtime score
				newItem->setRecordRuntimeScore(heapItem->geoElement->getScore(*this->queryShape));
				delete heapItem;
				return newItem;
			}
		}
		delete heapItem;
	}
	return NULL;
}

bool GeoNearestNeighborOperator::close(PhysicalPlanExecutionParameters & params){
	this->queryEvaluator = NULL;
	for( unsigned i = 0 ; i < this->heapItems.size() ; i++ ){
		delete this->heapItems[i];
	}
	this->heapItems.clear();
	this->queryShape = NULL;
	return true;
}

bool GeoNearestNeighborOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters){
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

string GeoNearestNeighborOperator::toString(){
	string result = "GeoNearestNeighborOperator";
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost GeoNearestNeighborOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost.cost = this->getLogicalPlanNode()->stats->quadTreeNodeSet.size();
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost GeoNearestNeighborOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	unsigned estimatedNumberOfleafNodes = this->getLogicalPlanNode()->stats->estimatedNumberOfLeafNodes;
	// cost of removing the item from the heap
	// TODO: consider the number of all geoelements in the heap
	resultCost.cost = log2((double)estimatedNumberOfleafNodes + GEO_MAX_NUM_OF_ELEMENTS + 1);
	return resultCost;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost GeoNearestNeighborOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost;
	unsigned estimatedNumberOfleafNodes = this->getLogicalPlanNode()->stats->estimatedNumberOfLeafNodes;
	resultCost.cost = estimatedNumberOfleafNodes + GEO_MAX_NUM_OF_ELEMENTS;
	return resultCost;
}

PhysicalPlanCost GeoNearestNeighborOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	// we only need to check if the query region contains the record's point or not
	PhysicalPlanCost resultCost;
	resultCost.cost = 1;
	return resultCost;
}

void GeoNearestNeighborOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_SortByScore);
}

void GeoNearestNeighborOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	prop.addProperty(PhysicalPlanIteratorProperty_LowestLevel);
}

PhysicalPlanNodeType GeoNearestNeighborOptimizationOperator::getType() {
	return PhysicalPlanNode_GeoNearestNeighbor;
}

bool GeoNearestNeighborOptimizationOperator::validateChildren(){
	// this operator cannot have any children
	if(getChildrenCount() > 0){
		return false;
	}
	return true;
}

}
}



