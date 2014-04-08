//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include <vector>
#include <algorithm>

#include "instantsearch/ResultsPostProcessor.h"
#include "SortByRefiningAttributeOperator.h"
#include "operation/QueryEvaluatorInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

bool SortByRefiningAttributeOperator::open(QueryEvaluatorInternal * queryEvaluatorInternal, PhysicalPlanExecutionParameters & params){
    ASSERT(sortEvaluator != NULL);
    ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
    if(sortEvaluator == NULL) return false;
    Schema * schema = queryEvaluatorInternal->getSchema();
    ForwardIndex * forwardIndex = queryEvaluatorInternal->getForwardIndex();

    shared_ptr<vectorview<ForwardListPtr> > readView;
    queryEvaluatorInternal->getForwardIndex_ReadView(readView);
    this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluatorInternal,params);
    // extract all the information from forward index
    // 1. find the participating attributes
    /*
     * Example : for example if the query contains "name,age,birthdate ASC" , the participating attributes are
     *           name, age and birthdate.
     */
    const vector<string> * attributes =
            sortEvaluator->getParticipatingAttributes();
    vector<unsigned> attributeIds;
    for (vector<string>::const_iterator attributeName = attributes->begin();
            attributeName != attributes->end(); ++attributeName) {
        unsigned id = schema->getRefiningAttributeId(*attributeName);
        attributeIds.push_back(id);
    }
    // 2. extract the data from forward index.
    while(true){
    	PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
    	if(nextRecord == NULL){
          break;
    	}
    	bool isValid = false;
	const ForwardList * list = forwardIndex->getForwardList(readView, nextRecord->getRecordId(), isValid);
	if (!isValid) // ignore the record if it's already deleted
          continue;
    	results.push_back(nextRecord);
	const Byte * refiningAttributesData =
			list->getRefiningAttributeContainerData();
	// now parse the values by VariableLengthAttributeContainer
	vector<TypedValue> typedValues;
	VariableLengthAttributeContainer::getBatchOfAttributes(attributeIds, schema , refiningAttributesData,&typedValues);
	// save the values in QueryResult objects
	for(std::vector<string>::const_iterator attributesIterator = attributes->begin() ;
			attributesIterator != attributes->end() ; ++attributesIterator){
		nextRecord->valuesOfParticipatingRefiningAttributes[*attributesIterator] =
				typedValues.at(std::distance(attributes->begin() , attributesIterator));
	}
    }

    // 3. now sort the results based on the comparator
    std::sort(results.begin(),
            results.end(),
            ResultRefiningAttributeComparator(this->sortEvaluator));


    cursorOnResults = 0;

    return true;
}
PhysicalPlanRecordItem * SortByRefiningAttributeOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	if(cursorOnResults >= results.size()){
		return NULL;
	}
	return results[cursorOnResults++];
}
bool SortByRefiningAttributeOperator::close(PhysicalPlanExecutionParameters & params){
	results.clear();
	cursorOnResults = 0;
	sortEvaluator = NULL;
    this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
    return true;
}

string SortByRefiningAttributeOperator::toString(){
	string result = "SortByRefiningAttributeOperator" + this->sortEvaluator->toString();
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool SortByRefiningAttributeOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	ASSERT(false);
	return false;
}
SortByRefiningAttributeOperator::~SortByRefiningAttributeOperator(){
	delete this->sortEvaluator;
}
SortByRefiningAttributeOperator::SortByRefiningAttributeOperator(SortEvaluator * sortEvaluator) {
	this->sortEvaluator = sortEvaluator;
}

// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost SortByRefiningAttributeOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost SortByRefiningAttributeOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost SortByRefiningAttributeOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	ASSERT(false);
	return PhysicalPlanCost(0);
}
PhysicalPlanCost SortByRefiningAttributeOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	ASSERT(false);
	return PhysicalPlanCost(0);
}
void SortByRefiningAttributeOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	ASSERT(false);
}
void SortByRefiningAttributeOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	ASSERT(false);
}
PhysicalPlanNodeType SortByRefiningAttributeOptimizationOperator::getType() {
	return PhysicalPlanNode_SortByRefiningAttribute;
}
bool SortByRefiningAttributeOptimizationOperator::validateChildren(){
	ASSERT(false);
	return false;
}

}
}
