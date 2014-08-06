/*
 * GeoSimpleScanOperator.h
 *
 *  Created on: Jul 9, 2014
 *      Author: mahdi
 */

#ifndef __GEOSIMPLESCANOPERATOR_H__
#define __GEOSIMPLESCANOPERATOR_H__

#include <vector>
#include <stdlib.h>
#include <util/Assert.h>

#include "operation/QueryEvaluatorInternal.h"
#include "operation/physical_plan/PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class GeoSimpleScanOperator : public PhysicalPlanNode {
	friend class PhysicalOperatorFactory;
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);

	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;

	bool close(PhysicalPlanExecutionParameters & params);

	string toString();

	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;

	~GeoSimpleScanOperator();

private:
	GeoSimpleScanOperator();

	//TODO: This pointer should change to a shared pointer
	QuadTree* quadtree;
	vector< vector<GeoElement*>* > geoElements;
	QueryEvaluatorInternal* queryEvaluator;
	unsigned vectorOffset; // keep the offset of the current reading vector in geoElements
	unsigned cursorOnVectorOfGeoElements; // keep the offset of the current reading geoElement
	Shape* queryShape;  // keep the shape of the query region
	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	unsigned latOffset;       // offset of the latitude attribute in the refining attribute memory
	unsigned longOffset;      // offset of the longitude attribute in the refining attribute memory
};

class GeoSimpleScanOptimizationOperator : public PhysicalPlanOptimizationNode{
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

private:
	QuadTree* quadtree;
};

}
}


#endif /* GEOSIMPLESCANOPERATOR_H_ */
