//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

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

#ifndef __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__
#define __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "instantsearch/TypedValue.h"
#include "operation/QueryEvaluatorInternal.h"

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
 * The main facet calculator which uses a list of FacetHelpers and a list of FacetContainers to prepare the facet results.
 */
class FacetedSearchFilterInternal
{

public:
    FacetedSearchFilterInternal(){
    }

    ~FacetedSearchFilterInternal(){
    	for(std::vector<FacetHelper *>::iterator facetHelperPtr = facetHelpers.begin() ;
    			facetHelperPtr != facetHelpers.end() ; ++facetHelperPtr){
    		if(*facetHelperPtr != NULL){
    			delete *facetHelperPtr;
    		}
    	}
    	for(std::vector<std::pair< FacetType , FacetResultsContainer * > >::iterator facetResultsPtr = facetResults.begin();
    			facetResultsPtr != facetResults.end(); ++facetResultsPtr){
    		if(facetResultsPtr->second != NULL){
    			delete facetResultsPtr->second;
    		}
    	}
    }

    void doFilter(QueryEvaluator *queryEvaluator,
            const Query * query, QueryResults * input, QueryResults * output) ;
    void preFilter(QueryEvaluator *queryEvaluator);
    void doProcessOneResult(const TypedValue & attributeValue, const unsigned facetFieldIndex);

    void initialize(std::vector<FacetType> & facetTypes,
            std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
            std::vector<std::string> & rangeEnds,
            std::vector<std::string> & rangeGaps , std::vector<int> & numberOfGroupsToReturn);

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

}
}

#endif // __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__
