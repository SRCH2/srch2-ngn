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
#include "instantsearch/LogicalPlan.h"
#include "util/Assert.h"
#include "operation/HistogramManager.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "sstream"

using namespace std;

namespace srch2 {
namespace instantsearch {


LogicalPlanNode::LogicalPlanNode(Term * exactTerm, Term * fuzzyTerm){
	this->nodeType = LogicalPlanNodeTypeTerm;
	this->exactTerm= exactTerm;
	this->fuzzyTerm = fuzzyTerm;
	this->regionShape = NULL;
	/*
	 * This member is allocated in HistogramManager::allocateLogicalPlanNodeAnnotations
	 * which is called in HistogramManager::annotate
	 * and is deallocated in two places:
	 * 1. HistogramManager::freeStatsOfLogicalPlanTree which is called in the
	 *    fuzzy round of KeywordSearchOperator open method (allocated for exact round)
	 * 2. Destructor of this class (allocated for the last round, exact for only exact queries and
	 *    fuzzy for fuzzy queries).
	 */
	this->stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	ASSERT(nodeType != LogicalPlanNodeTypeGeo);
	this->nodeType = nodeType;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
	this->regionShape = NULL;
	/*
	 * This member is allocated in HistogramManager::allocateLogicalPlanNodeAnnotations
	 * which is called in HistogramManager::annotate
	 * and is deallocated in two places:
	 * 1. HistogramManager::freeStatsOfLogicalPlanTree which is called in the
	 *    fuzzy round of KeywordSearchOperator open method (allocated for exact round)
	 * 2. Destructor of this class (allocated for the last round, exact for only exact queries and
	 *    fuzzy for fuzzy queries).
	 */
	this->stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(Shape* regionShape){
	this->nodeType = LogicalPlanNodeTypeGeo;
	this->exactTerm = NULL;
	this->fuzzyTerm = NULL;
	this->regionShape = regionShape;
	/*
	 * This member is allocated in HistogramManager::allocateLogicalPlanNodeAnnotations
	 * which is called in HistogramManager::annotate
	 * and is deallocated in two places:
	 * 1. HistogramManager::freeStatsOfLogicalPlanTree which is called in the
	 *    fuzzy round of KeywordSearchOperator open method (allocated for exact round)
	 * 2. Destructor of this class (allocated for the last round, exact for only exact queries and
	 *    fuzzy for fuzzy queries).
	 */
	this->stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::~LogicalPlanNode(){
	if(this->exactTerm != NULL){
		delete exactTerm;
	}
	if(this->fuzzyTerm != NULL){
		delete fuzzyTerm;
	}
	if(this->regionShape != NULL){
		delete regionShape;
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

string LogicalPlanNode::toString(){
	stringstream ss;
	ss << this->nodeType;
	if(this->exactTerm != NULL){
		ss << this->exactTerm->toString();
	}
	if(this->fuzzyTerm != NULL){
		ss << this->fuzzyTerm->toString();
	}
	if(this->regionShape != NULL){
		ss << this->regionShape->toString();
	}
	ss << this->forcedPhysicalNode;
	return ss.str();
}

string LogicalPlanNode::getSubtreeUniqueString(){

	string result = this->toString();
	for(unsigned childOffset = 0 ; childOffset < this->children.size() ; ++childOffset){
		ASSERT(this->children.at(childOffset) != NULL);
		result += this->children.at(childOffset)->getSubtreeUniqueString();
	}
	return result;
}


//////////////////////////////////////////////// Logical Plan ///////////////////////////////////////////////

LogicalPlan::LogicalPlan(){
	tree = NULL;
	postProcessingInfo = NULL;
	fuzzyQuery = exactQuery = NULL;
	postProcessingPlan = NULL;
}

LogicalPlan::~LogicalPlan(){
	if(tree != NULL) delete tree;
	if(postProcessingInfo != NULL){
		delete postProcessingInfo;
	}
	delete postProcessingPlan;
	delete fuzzyQuery; delete exactQuery;
}

LogicalPlanNode * LogicalPlan::createTermLogicalPlanNode(const std::string &queryKeyword,
		TermType type,const float boost, const float fuzzyMatchPenalty,
		const uint8_t threshold , const vector<unsigned>& fieldFilter,ATTRIBUTES_OP attrOp){
	Term * term = new Term(queryKeyword, type, boost, fuzzyMatchPenalty, threshold);
	term->addAttributesToFilter(fieldFilter, attrOp);
	LogicalPlanNode * node = new LogicalPlanNode(term , NULL);
	return node;
}

LogicalPlanNode * LogicalPlan::createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	ASSERT(nodeType != LogicalPlanNodeTypeGeo);
	LogicalPlanNode * node = new LogicalPlanNode(nodeType);
	return node;
}
LogicalPlanNode * LogicalPlan::createPhraseLogicalPlanNode(const vector<string>& phraseKeyWords,
		const vector<unsigned>& phraseKeywordsPosition,
		short slop, const vector<unsigned>& fieldFilter, ATTRIBUTES_OP attrOp) {

	LogicalPlanNode * node = new LogicalPlanPhraseNode(phraseKeyWords, phraseKeywordsPosition,
			slop, fieldFilter, attrOp);
	return node;
}

LogicalPlanNode * LogicalPlan::createGeoLogicalPlanNode(Shape *regionShape){
	LogicalPlanNode * node = new LogicalPlanNode(regionShape);
	return node;
}


}
}
