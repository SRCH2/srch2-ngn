
#include "PhysicalPlan.h"
#include "operation/QueryEvaluatorInternal.h"
#include "util/QueryOptimizerUtil.h"

using namespace std;

namespace srch2 {
namespace instantsearch {



// This function is called to determine whether the output properties of an operator
// match the input properties of another one.
bool IteratorProperties::isMatchAsInputTo(const IteratorProperties & prop , IteratorProperties & reason){
	bool result = true;
	for(vector<PhysicalPlanIteratorProperty>::const_iterator property = prop.properties.begin(); property != prop.properties.end() ; ++property){
		if(find(this->properties.begin(),this->properties.end(), *property) == this->properties.end()){
			reason.addProperty(*property);
			result = false;
		}
	}

	return result;
}

// Adds this property to the list of properties that this object has
void IteratorProperties::addProperty(PhysicalPlanIteratorProperty prop){
	this->properties.push_back(prop);
}



void PhysicalPlanOptimizationNode::setChildAt(unsigned offset, PhysicalPlanOptimizationNode * child) {
	if(offset >= children.size()){
		ASSERT(false);
		return;
	}
	children.at(offset) = child;
}

void PhysicalPlanOptimizationNode::addChild(PhysicalPlanOptimizationNode * child) {
	if(child == NULL){
		ASSERT(false);
		return;
	}
	children.push_back(child);
}


// parent is not set in the code, it's going to be removed if not needed TODO
void PhysicalPlanOptimizationNode::setParent(PhysicalPlanOptimizationNode * parent){
	this->parent = parent;
}

PhysicalPlanOptimizationNode * PhysicalPlanOptimizationNode::getParent(){
	return parent;
}

void PhysicalPlanOptimizationNode::printSubTree(unsigned indent){
	PhysicalPlanNodeType type = getType();
	srch2::util::QueryOptimizerUtil::printIndentations(indent);
	switch (type) {
		case PhysicalPlanNode_SortById:
//			Logger::info("[SortByID]");
			cout << "[SortByID]" << endl;
			break;
		case PhysicalPlanNode_SortByScore:
//			Logger::info("[SortByScore]");
			cout << "[SortByScore]" << endl;
			break;
		case PhysicalPlanNode_MergeTopK:
//			Logger::info("[AND TopK]");
			cout << "[AND TopK]" << endl;
			break;
		case PhysicalPlanNode_MergeSortedById:
//			Logger::info("[AND SortedByID]");
			cout << "[AND SortedByID]" << endl;
			break;
		case PhysicalPlanNode_MergeByShortestList:
//			Logger::info("[AND ShortestList]");
			cout << "[AND ShortestList]" << endl;
			break;
		case PhysicalPlanNode_UnionSortedById:
			Logger::info("[OR SortedByID]");
			cout << "[OR SortedByID]" << endl;
			break;
		case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
//			Logger::info("[TVL]");
			cout << "[TVL]" << endl;
			break;
		case PhysicalPlanNode_UnionLowestLevelSimpleScanOperator:
//			Logger::info("[SCAN]");
			cout << "[SCAN]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessTerm:
//			Logger::info("[TERM]" );
			cout << "[TERM]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessAnd:
//			Logger::info("[R.A.AND]");
			cout << "[R.A.AND]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessOr:
//			Logger::info("[R.A.OR]");
			cout << "[R.A.OR]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessNot:
//			Logger::info("[R.A.NOT]");
			cout << "[R.A.NOT]" << endl;
			break;
	}
	for(vector<PhysicalPlanOptimizationNode *>::iterator child = children.begin(); child != children.end() ; ++child){
		(*child)->printSubTree(indent+1);
	}
}

void PhysicalPlanNode::setPhysicalPlanOptimizationNode(PhysicalPlanOptimizationNode * optimizationNode){
	this->optimizationNode = optimizationNode;
}

PhysicalPlan::PhysicalPlan(	QueryEvaluatorInternal * queryEvaluator){
	this->queryEvaluator = 	queryEvaluator;
	this->tree = NULL;
	this->executionParameters = NULL;
}

PhysicalPlan::~PhysicalPlan(){
//	if(tree != NULL) delete tree;
	if(executionParameters != NULL) delete executionParameters;
}



//ForwardIndex * PhysicalPlan::getForwardIndex(){
//	return this->queryEvaluator->getForwardIndex();
//}
//
//const InvertedIndex * PhysicalPlan::getInvertedIndex(){
//	return this->queryEvaluator->getInvertedIndex();
//}

PhysicalPlanNode * PhysicalPlan::getPlanTree(){
	return this->tree;
}

void PhysicalPlan::setPlanTree(PhysicalPlanNode * tree){
	this->tree = tree;
}

void PhysicalPlan::setSearchType(srch2is::QueryType searchType){
	this->searchType = searchType;
}

srch2is::QueryType PhysicalPlan::getSearchType(){
	return this->searchType;
}

void PhysicalPlan::setExecutionParameters(PhysicalPlanExecutionParameters * executionParameters){
	ASSERT(this->executionParameters == NULL && executionParameters != NULL);
	this->executionParameters = executionParameters;
}

PhysicalPlanExecutionParameters * PhysicalPlan::getExecutionParameters(){
	ASSERT(this->executionParameters != NULL);
	return this->executionParameters;
}


}}
