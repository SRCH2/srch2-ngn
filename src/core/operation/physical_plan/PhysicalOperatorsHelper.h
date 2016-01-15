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
