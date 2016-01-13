
#ifndef __PHYSICALPLAN_PHYSICALOPERATORSHELPER_H__
#define __PHYSICALPLAN_PHYSICALOPERATORSHELPER_H__

#include "instantsearch/Term.h"
#include "operation/ActiveNode.h"
#include "operation/QueryEvaluatorInternal.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


/*
 * The following 4 functions are helper functions which are used in verifyByRandomAccess() implementation of
 * different operators. The first function (right below) is the main one which accesses forward index to see if
 * the record contains a term or not.
 */

bool verifyByRandomAccessHelper(QueryEvaluatorInternal * queryEvaluator,
		PrefixActiveNodeSet *prefixActiveNodeSet,
		Term * term,
		PhysicalPlanRandomAccessVerificationParameters & parameters);



bool verifyByRandomAccessAndHelper(PhysicalPlanOptimizationNode * node, PhysicalPlanRandomAccessVerificationParameters & parameters);

bool verifyByRandomAccessOrHelper(PhysicalPlanOptimizationNode * node, PhysicalPlanRandomAccessVerificationParameters & parameters);

bool verifyByRandomAccessGeoHelper(PhysicalPlanRandomAccessVerificationParameters & parameters, QueryEvaluatorInternal * queryEvaluator, Shape* queryShape, unsigned &latOffset, unsigned &longOffset);

// this function finds the offset of the latitude and longitude attributes in the refining attributes memory
void getLat_Long_Offset(unsigned & latOffset, unsigned & longOffset, const Schema * schema);

}
}
#endif // __PHYSICALPLAN_PHYSICALOPERATORSHELPER_H__
