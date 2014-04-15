
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

#ifndef __PHYSICALPLAN_FACETOPERATOR_H__
#define __PHYSICALPLAN_FACETOPERATOR_H__

#include "instantsearch/Constants.h"
#include "index/ForwardIndex.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "operation/HistogramManager.h"
#include "PhysicalPlan.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

/*
 * This class and its two children (CategoricalFacetHelper and RangeFacetHelper)
 * are responsible of generating ID and name of a bucket. It is given all the information
 * (e.g. start end and gap) and also the record data and it will decide which bucket that
 * data point should go in.
 */
class FacetHelper
{
public:
	virtual std::pair<unsigned , std::string> generateIDAndName(const TypedValue & attributeValue) = 0;
	void generateIDAndNameForMultiValued(const TypedValue & attributeValue,
			std::vector< std::pair<unsigned , std::string> > & resultIdsAndNames);
	virtual void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) = 0;
	virtual void initialize(const std::string * facetInfoForInitialization , const Schema * schema) = 0;

	virtual ~FacetHelper(){};
};

/*
 * FacetHelper for Categorical Facets. It extract all distinctive values of an attribute and
 * gives bucketIDs based on those values.
 */
class CategoricalFacetHelper : public FacetHelper
{
public:
	std::pair<unsigned , std::string> generateIDAndName(const TypedValue & attributeValue) ;
	void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) ;
	void initialize(const std::string * facetInfoForInitialization , const Schema * schema) ;

	std::map<std::string, unsigned> categoryValueToBucketIdMap;
};

/*
 * FacetHelper for Range Facets. Based on start,end and gap it decides on the bucketID of a data point.
 */
class RangeFacetHelper : public FacetHelper
{
public:
	RangeFacetHelper(){
		generateListOfIdsAndNamesFlag = false;
	}
	std::pair<unsigned , std::string> generateIDAndName(const TypedValue & attributeValue) ;
	void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) ;
	void initialize(const std::string * facetInfoForInitialization , const Schema * schema) ;

private:
	TypedValue start, end, gap;
	unsigned numberOfBuckets;
	bool generateListOfIdsAndNamesFlag;
	FilterType attributeType;
};

////////////////////////////////////////////////////////////////////////////////
// FacetResultsContainer and its two children
/*
 * FacetResultsContainer and its two children (CategoricalFacetResultsContainer and RangeFacetResultsContainer)
 * are the containers of the buckets. They keep the buckets of a facet and also apply aggregation operations.
 */
class FacetResultsContainer
{
public:
	virtual void initialize(FacetHelper * facetHelper , FacetAggregationType  aggregationType) = 0;
	virtual void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType aggregationType) = 0;
	virtual void getNamesAndValues(std::vector<std::pair< std::string, float > > & results , int numberOfGroupsToReturn = -1) = 0;
	virtual ~FacetResultsContainer() {};

	float initAggregation(FacetAggregationType  aggregationType);
	float doAggregation(float bucketValue, FacetAggregationType  aggregationType);
};

/*
 * Bucket container for Categorical facets.
 */
class CategoricalFacetResultsContainer : public FacetResultsContainer
{
public:
	// a map from bucket id to the pair of bucketname,bucketvalue
	std::map<unsigned, std::pair<std::string, float> > bucketsInfo;

	void initialize(FacetHelper * facetHelper , FacetAggregationType  aggregationType);
	void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType aggregationType);
	void getNamesAndValues(std::vector<std::pair< std::string, float > > & results, int numberOfGroupsToReturn = -1);

};

/*
 * Bucket container for Range facets.
 */
class RangeFacetResultsContainer : public FacetResultsContainer
{
public:
	// A list of pairs of <bucketname,bucketvalue> that bucketId is the index of each bucket in this list
	std::vector<std::pair<std::string, float> > bucketsInfo;

	void initialize(FacetHelper * facetHelper , FacetAggregationType  aggregationType);
	void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType aggregationType);
	void getNamesAndValues(std::vector<std::pair< std::string, float > > & results, int numberOfGroupsToReturn = -1);

};

/*
 * The following two classes implement Facet as a physical operator.
 * this operator is always on top of the root physical operator and it computes facets
 * while retrieving all records from its child and returning them to its parent
 * (which might be another post processing operator) untouched.
 * Example :
 * q = A AND B OR C & facet=true & facet.field=model
 * the core plan for query execution (one possible plan):
 * [OR sorted by ID]___[SORT BY ID]___[SCAN C]
 *       |
 *       |_____________[SORT BY ID]___[Merge TopK]____[TVL A]
 *                                          |_________[TVL B]
 *
 * but the complete plan tree looks like this :
 *
 * [FacetOperator]
 *        |
 * [KeywordSearchOperator]
 * { // the core plan is built and optimized in open function of KeywordSearchOperator
 * [OR sorted by ID]___[SORT BY ID]___[SCAN C]
 *       |
 *       |_____________[SORT BY ID]___[Merge TopK]____[TVL A]
 *                                          |_________[TVL B]
 * }
 *
 * and the results are collected by calling getNext of FacetOperator iteratively .
 *
 */
class FacetOperator : public PhysicalPlanNode {
public:
	bool open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params);
	PhysicalPlanRecordItem * getNext(const PhysicalPlanExecutionParameters & params) ;
	bool close(PhysicalPlanExecutionParameters & params);
	string toString();
	bool verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) ;
	void getFacetResults(QueryResults * output);
	~FacetOperator();
	FacetOperator(QueryEvaluatorInternal *queryEvaluator, std::vector<FacetType> & facetTypes,
	        std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
	        std::vector<std::string> & rangeEnds,
	        std::vector<std::string> & rangeGaps, std::vector<int> & numberOfGroupsToReturn) ;
private:
	void preFilter(QueryEvaluatorInternal *queryEvaluator);
	void doProcessOneResult(const TypedValue & attributeValue, const unsigned facetFieldIndex);

	QueryEvaluatorInternal *queryEvaluatorInternal;

    std::vector<FacetType> facetTypes;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;
    std::vector<int> numberOfGroupsToReturnVector;

    // These two vectors are parallel with fields.
	std::vector<FacetHelper *> facetHelpers;
	std::vector<std::pair< FacetType , FacetResultsContainer * > > facetResults;
};

class FacetOptimizationOperator : public PhysicalPlanOptimizationNode {
public:
	// The cost of open of a child is considered only once in the cost computation
	// of parent open function.
	PhysicalPlanCost getCostOfOpen(const PhysicalPlanExecutionParameters & params) ;
	// The cost of getNext of a child is multiplied by the estimated number of calls to this function
	// when the cost of parent is being calculated.
	PhysicalPlanCost getCostOfGetNext(const PhysicalPlanExecutionParameters & params) ;
	// the cost of close of a child is only considered once since each node's close function is only called once.
	PhysicalPlanCost getCostOfClose(const PhysicalPlanExecutionParameters & params) ;
	PhysicalPlanCost getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params);
	void getOutputProperties(IteratorProperties & prop);
	void getRequiredInputProperties(IteratorProperties & prop);
	PhysicalPlanNodeType getType() ;
	bool validateChildren();
};

}
}

#endif //__PHYSICALPLAN_FACETOPERATOR_H__
