

#include "FilterQueryOperator.h"
#include "PhysicalOperators.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "operation/QueryEvaluatorInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/TypedValue.h"
#include <vector>
#include "string"

namespace srch2
{
namespace instantsearch
{


bool FilterQueryOperator::open(QueryEvaluatorInternal * queryEvaluatorInternal, PhysicalPlanExecutionParameters & params){
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluatorInternal, params);
	this->queryEvaluatorInternal = queryEvaluatorInternal;
	return true;
}
PhysicalPlanRecordItem * FilterQueryOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	while(true){
		PhysicalPlanRecordItem * nextRecord = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
		if(nextRecord == NULL){
			return NULL;
		}
		Schema * schema = queryEvaluatorInternal->getSchema();
		ForwardIndex * forwardIndex = queryEvaluatorInternal->getForwardIndex();
		if(doPass(schema, forwardIndex, nextRecord)){
			return nextRecord;
		}
	}
	ASSERT(false); // we can never reach to this point
	return NULL;
}
bool FilterQueryOperator::close(PhysicalPlanExecutionParameters & params){
	this->filterQueryEvaluator = NULL;
	this->queryEvaluatorInternal = NULL;
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

string FilterQueryOperator::toString(){
	string result = "filterQueryOperator" + this->filterQueryEvaluator->toString() ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}

bool FilterQueryOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	return this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
}
FilterQueryOperator::~FilterQueryOperator(){}

FilterQueryOperator::FilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator) {
	this->filterQueryEvaluator = filterQueryEvaluator;
}

bool FilterQueryOperator::doPass(Schema * schema, ForwardIndex * forwardIndex ,PhysicalPlanRecordItem * record){
    // fetch the names and ids of non searchable attributes from schema
    vector<string> attributes;
    vector<unsigned> attributeIds;
    for(map<string,unsigned>::const_iterator attr = schema->getRefiningAttributes()->begin();
            attr != schema->getRefiningAttributes()->end() ; ++attr ){
        attributes.push_back(attr->first);
        attributeIds.push_back(attr->second);
    }

    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->queryEvaluatorInternal->getForwardIndex_ReadView(readView);
    // now fetch the values of different attributes from forward index
    vector<TypedValue> typedValues;
    bool isValid = false;
    const ForwardList * list = forwardIndex->getForwardList(readView, record->getRecordId() , isValid);
    ASSERT(isValid);
    const Byte * refiningAttributesData = list->getRefiningAttributeContainerData();
    VariableLengthAttributeContainer::getBatchOfAttributes(attributeIds,schema,refiningAttributesData ,&typedValues);

    // now call the evaluator to see if this record passes the criteria or not
    // A criterion can be for example price:12 or price:[* TO 100]
    map<string, TypedValue> valuesForEvaluation;
    // prepare the evaluator input
    unsigned scoresIndex =0;
    for(vector<string>::iterator attr = attributes.begin() ; attr != attributes.end() ; ++attr ){
        valuesForEvaluation[*attr] = typedValues.at(scoresIndex);
        scoresIndex++;
    }
    return this->filterQueryEvaluator->evaluate(valuesForEvaluation);
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfOpen(params);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfGetNext(params) + 1;
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	return this->getChildAt(0)->getCostOfClose(params);
}
PhysicalPlanCost FilterQueryOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	return this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
}
void FilterQueryOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	return;
}
void FilterQueryOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	return ;
}
PhysicalPlanNodeType FilterQueryOptimizationOperator::getType() {
	return PhysicalPlanNode_FilterQuery;
}
bool FilterQueryOptimizationOperator::validateChildren(){
	return true;
}

}
}
