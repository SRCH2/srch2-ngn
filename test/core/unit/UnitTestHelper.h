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

#ifndef __UNITTESTHELPER_H__
#define __UNITTESTHELPER_H__

#include <stdlib.h>
using namespace std;
using namespace srch2::instantsearch;


LogicalPlan * prepareLogicalPlanForUnitTests(Query * exactQuery, Query * fuzzyQuery,
		int offset, int resultsToRetrieve, bool shouldRunFuzzyQuery, srch2::instantsearch::QueryType searchType){
	LogicalPlan * logicalPlan = new LogicalPlan();
	logicalPlan->exactQuery = exactQuery;
	logicalPlan->fuzzyQuery = fuzzyQuery;
	logicalPlan->numberOfResultsToRetrieve = resultsToRetrieve;
	logicalPlan->offset = offset;
	logicalPlan->shouldRunFuzzyQuery = shouldRunFuzzyQuery;
	logicalPlan->queryType = searchType;

	logicalPlan->setTree(logicalPlan->createOperatorLogicalPlanNode(LogicalPlanNodeTypeAnd));

	for(unsigned t = 0 ; t < exactQuery->getQueryTerms()->size() ; t++){
		LogicalPlanNode * newNode = logicalPlan->createTermLogicalPlanNode(
				*(exactQuery->getQueryTerms()->at(t)->getKeyword()),
				exactQuery->getQueryTerms()->at(t)->getTermType(),
				exactQuery->getQueryTerms()->at(t)->getBoost(),
				exactQuery->getQueryTerms()->at(t)->getSimilarityBoost(),
				exactQuery->getQueryTerms()->at(t)->getThreshold(),
				exactQuery->getQueryTerms()->at(t)->getAttributesToFilter(),
				exactQuery->getQueryTerms()->at(t)->getFilterAttrOperation());
		if(shouldRunFuzzyQuery){
			newNode->setFuzzyTerm(fuzzyQuery->getQueryTerms()->at(t));
		}
		logicalPlan->getTree()->children.push_back(newNode);
	}

	return logicalPlan;
}

LogicalPlan * prepareLogicalPlanForGeoTest(Query * exactQuery, Query * fuzzyQuery, int offset,
		int resultsToRetrieve, bool shouldRunFuzzyQuery, srch2::instantsearch::QueryType searchType){
	LogicalPlan * logicalPlan = new LogicalPlan();
	logicalPlan->exactQuery = exactQuery;
	logicalPlan->fuzzyQuery = fuzzyQuery;
	logicalPlan->numberOfResultsToRetrieve = resultsToRetrieve;
	logicalPlan->offset = offset;
	logicalPlan->shouldRunFuzzyQuery = shouldRunFuzzyQuery;
	logicalPlan->queryType = searchType;

	logicalPlan->setTree(logicalPlan->createOperatorLogicalPlanNode(LogicalPlanNodeTypeAnd));

	for(unsigned t = 0 ; t < exactQuery->getQueryTerms()->size() ; t++){
		LogicalPlanNode * newNode = logicalPlan->createTermLogicalPlanNode(
						*(exactQuery->getQueryTerms()->at(t)->getKeyword()),
						exactQuery->getQueryTerms()->at(t)->getTermType(),
						exactQuery->getQueryTerms()->at(t)->getBoost(),
						exactQuery->getQueryTerms()->at(t)->getSimilarityBoost(),
						exactQuery->getQueryTerms()->at(t)->getThreshold(),
						exactQuery->getQueryTerms()->at(t)->getAttributesToFilter(),
						exactQuery->getQueryTerms()->at(t)->getFilterAttrOperation());
		if(shouldRunFuzzyQuery){
			newNode->setFuzzyTerm(fuzzyQuery->getQueryTerms()->at(t));
		}
		logicalPlan->getTree()->children.push_back(newNode);
	}
	LogicalPlanNode * newNode = logicalPlan->createGeoLogicalPlanNode(exactQuery->getShape());
	logicalPlan->getTree()->children.push_back(newNode);

	return logicalPlan;
}

LogicalPlan * prepareLogicalPlanForACLTests(Query * exactQuery, Query * fuzzyQuery,
		int offset, int resultsToRetrieve, bool shouldRunFuzzyQuery, srch2::instantsearch::QueryType searchType, string& roleId){
	LogicalPlan * logicalPlan = new LogicalPlan();
	logicalPlan->exactQuery = exactQuery;
	logicalPlan->fuzzyQuery = fuzzyQuery;
	logicalPlan->numberOfResultsToRetrieve = resultsToRetrieve;
	logicalPlan->offset = offset;
	logicalPlan->shouldRunFuzzyQuery = shouldRunFuzzyQuery;
	logicalPlan->queryType = searchType;

	logicalPlan->setTree(logicalPlan->createOperatorLogicalPlanNode(LogicalPlanNodeTypeAnd));

	for(unsigned t = 0 ; t < exactQuery->getQueryTerms()->size() ; t++){
		LogicalPlanNode * newNode = logicalPlan->createTermLogicalPlanNode(
				*(exactQuery->getQueryTerms()->at(t)->getKeyword()),
				exactQuery->getQueryTerms()->at(t)->getTermType(),
				exactQuery->getQueryTerms()->at(t)->getBoost(),
				exactQuery->getQueryTerms()->at(t)->getSimilarityBoost(),
				exactQuery->getQueryTerms()->at(t)->getThreshold(),
				exactQuery->getQueryTerms()->at(t)->getAttributesToFilter(),
				exactQuery->getQueryTerms()->at(t)->getFilterAttrOperation());
		if(shouldRunFuzzyQuery){
			newNode->setFuzzyTerm(fuzzyQuery->getQueryTerms()->at(t));
		}
		logicalPlan->getTree()->children.push_back(newNode);
	}
	logicalPlan->setPostProcessingInfo(new ResultsPostProcessingInfo());
	logicalPlan->getPostProcessingInfo()->setRoleId(roleId);

	return logicalPlan;
}


#endif // __UNITTESTHELPER_H__
