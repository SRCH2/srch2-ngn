

#include "instantsearch/LogicalPlan.h"

#include "util/Assert.h"
#include "operation/HistogramManager.h"
#include "instantsearch/ResultsPostProcessor.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


LogicalPlanNode::LogicalPlanNode(Term * exactTerm, Term * fuzzyTerm){
	this->nodeType = LogicalPlanNodeTypeTerm;
	this->exactTerm= exactTerm;
	this->fuzzyTerm = fuzzyTerm;
	stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	this->nodeType = nodeType;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
	stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::~LogicalPlanNode(){
	if(this->exactTerm != NULL){
		delete exactTerm;
	}
	if(this->fuzzyTerm != NULL){
		delete fuzzyTerm;
	}
	for(vector<LogicalPlanNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
		if(*child != NULL){
			delete *child;
		}
	}
	if(stats != NULL) delete stats;
}

void LogicalPlanNode::setFuzzyTerm(Term * fuzzyTerm){
	this->fuzzyTerm = fuzzyTerm;
}

//////////////////////////////////////////////// Logical Plan ///////////////////////////////////////////////

LogicalPlan::LogicalPlan(){
	tree = NULL;
	postProcessingInfo = NULL;
}

LogicalPlan::~LogicalPlan(){
	if(tree != NULL) delete tree;
	if(postProcessingInfo != NULL){
		delete postProcessingInfo;
	}
}

LogicalPlanNode * LogicalPlan::createTermLogicalPlanNode(const std::string &queryKeyword, TermType type,const float boost, const float fuzzyMatchPenalty, const uint8_t threshold , unsigned fieldFilter){
	Term * term = new Term(queryKeyword, type, boost, fuzzyMatchPenalty, threshold);
	term->addAttributeToFilterTermHits(fieldFilter);
	LogicalPlanNode * node = new LogicalPlanNode(term , NULL);
	return node;
}

LogicalPlanNode * LogicalPlan::createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	LogicalPlanNode * node = new LogicalPlanNode(nodeType);
	return node;
}


}
}
