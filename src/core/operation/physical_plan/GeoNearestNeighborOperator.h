/*
 * GeoNearestNeighborOperator.h
 *
 *  Created on: Jul 9, 2014
 *      Author: mahdi
 */

#ifndef __GEONEARESTNEIGHBOROPERATOR_H__
#define __GEONEARESTNEIGHBOROPERATOR_H__

#include <vector>
#include <stdlib.h>

#include "operation/QueryEvaluatorInternal.h"
#include "operation/physical_plan/PhysicalPlan.h"
#include "util/AndroidMath.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

// Using this class to store pointers to quadtree nodes or geoElements inside the heap vector in GeoNearestNeighborOperator class.
class GeoNearestNeighborOperatorHeapItem{
public:
	bool isQuadTreeNode; // this shows that this class contains a pointer to a QuadTreeNode or a GeoElement

	// Based on the 'isQuadTreeNode' flag one of these pointers is valid.
	QuadTreeNode* quadtreeNode;
	GeoElement* geoElement;

	double distFromQuery; // Distance of this node from the center of the query

	GeoNearestNeighborOperatorHeapItem(QuadTreeNode* quadtreeNode, Shape* queryShape){
		this->isQuadTreeNode = true;
		this->quadtreeNode = quadtreeNode;
		this->geoElement = NULL;
		Point queryCenter;
		queryShape->getCenter(queryCenter);
		this->distFromQuery = quadtreeNode->getRectangle().getMinDistFromBoundary(queryCenter.x, queryCenter.y);
	}

	GeoNearestNeighborOperatorHeapItem(GeoElement* geoElement, Shape* queryShape){
		this->isQuadTreeNode = false;
		this->geoElement = geoElement;
		this->quadtreeNode = NULL;
		Point queryCenter;
		queryShape->getCenter(queryCenter);
		this->distFromQuery = sqrt(geoElement->point.distSquare(queryCenter));
	}

	~GeoNearestNeighborOperatorHeapItem(){};

};

class GeoNearestNeighborOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:

	struct GeoNearestNeighborOperatorHeapItemCmp{
		GeoNearestNeighborOperatorHeapItemCmp(){};
		bool operator() (const GeoNearestNeighborOperatorHeapItem *lhs, const GeoNearestNeighborOperatorHeapItem *rhs) const {
			return lhs->distFromQuery > rhs->distFromQuery;
		}
	};

	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);

	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;

	bool close(PhysicalPlanExecutionParameters & params);

	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;

	string toString();

	~GeoNearestNeighborOperator();

private:
	GeoNearestNeighborOperator();

	// Min Heap to find the nearest neighbor of the query point in the quadtree.
	vector< GeoNearestNeighborOperatorHeapItem* >  heapItems;
	QueryEvaluatorInternal* queryEvaluator;
	boost::shared_ptr<GeoBusyNodeSet> quadTreeNodeSetSharedPtr;
	Shape* queryShape;  // keep the shape of the query region
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	unsigned latOffset;       // offset of the latitude attribute in the refining attribute memory
	unsigned longOffset;      // offset of the longitude attribute in the refining attribute memory
};

class GeoNearestNeighborOptimizationOperator : public PhysicalPlanOptimizationNode{
	friend class PhysicalOperatorFactory;
public:
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params);

	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;

	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;

	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);

	void getOutputProperties(IteratorProperties & prop);

	void getRequiredInputProperties(IteratorProperties & prop);

	PhysicalPlanNodeType getType() ;

	bool validateChildren();
};

}
}


#endif /* __GEONEARESTNEIGHBOROPERATOR_H__ */
