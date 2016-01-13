
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
