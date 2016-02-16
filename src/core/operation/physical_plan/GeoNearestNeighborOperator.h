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
