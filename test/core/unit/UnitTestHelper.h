//$Id: IntegrationTestHelper.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#ifndef __UNITTESTHELPER_H__
#define __UNITTESTHELPER_H__



using namespace srch2::instantsearch;


LogicalPlan * prepareLogicalPlanForUnitTests(Query * exactQuery, Query * fuzzyQuery ,
																			int offset,
																			int resultsToRetrieve,
																			bool shouldRunFuzzyQuery,
																			srch2::instantsearch::QueryType searchType){
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


#endif // __UNITTESTHELPER_H__
