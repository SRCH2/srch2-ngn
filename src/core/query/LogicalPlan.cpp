

#include "instantsearch/LogicalPlan.h"

#include "util/Assert.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


LogicalPlanNode::LogicalPlanNode(Term * term){
	this->nodeType = LogicalPlanNodeTypeTerm;
	this->term = term;
}

LogicalPlanNode::LogicalPlanNode(LogicalPlanNodeType nodeType){
	srch2::util::ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	this->nodeType = nodeType;
	this->term = NULL;
}

LogicalPlanNode::LogicalPlanNode(){
	this->nodeType = LogicalPlanNodeTypePlaceHolder;
	this->term = NULL;
}

LogicalPlanNode::~LogicalPlanNode(){
	if(this->term != NULL){
		delete term;
	}
	for(vector<LogicalPlanNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
		if(*child != NULL){
			delete *child;
		}
	}
}

void LogicalPlanNode::changeToTermLogicalPlanNode(
		LogicalPlanNode * logicalPlanNode,
		const std::string &queryKeyword,
		TermType type,
		const float boost,
		const float similarityBoost,
		const uint8_t threshold,
		unsigned fieldFilter){

	ASSERT(this->nodeType == LogicalPlanNodeTypePlaceHolder);
	Term * term = Term(queryKeyword, type, boost, similarityBoost, threshold);
	term->addAttributeToFilterTermHits(fieldFilter);
	this->term = term;
	this->nodeType = LogicalPlanNodeTypeTerm;
}

unsigned LogicalPlanNode::getNodeId(){
	return this->nodeId;
}
//////////////////////////////////////////////// Logical Plan ///////////////////////////////////////////////

LogicalPlan::LogicalPlan(){
	tree = NULL;
	logicalPlanNodeId = 0;
}

LogicalPlan::~LogicalPlan(){
	if(tree != NULL) delete tree;
}

LogicalPlanNode * LogicalPlan::createTermLogicalPlanNode(const std::string &queryKeyword, TermType type,const float boost, const float similarityBoost, const uint8_t threshold , unsigned fieldFilter){
	Term * term = Term(queryKeyword, type, boost, similarityBoost, threshold);
	term->addAttributeToFilterTermHits(fieldFilter);
	LogicalPlanNode node = new LogicalPlanNode(term);
	node->nodeId = ++logicalPlanNodeId;
	return node;
}

LogicalPlanNode * LogicalPlan::createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType){
	srch2::util::ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	LogicalPlanNode node = new LogicalPlanNode(nodeType);
	node->nodeId = ++logicalPlanNodeId;
	return node;
}

LogicalPlanNode * LogicalPlan::createDummyLogicalPlanNode(){
	LogicalPlanNode node = new LogicalPlanNode();
	node->nodeId = ++logicalPlanNodeId;
	return node;
}


}
}
