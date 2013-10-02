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
#include "instantsearch/Score.h"
#include "operation/IndexSearcherInternal.h"

namespace srch2 {
namespace instantsearch {


class FacetHelper
{
public:
	virtual std::pair<unsigned , std::string> generateIDAndName(const Score & attributeValue) = 0;
	virtual void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) = 0;
	virtual void initialize(const std::string * info , const Schema * schema) = 0;

	virtual ~FacetHelper(){};
};

class CategoricalFacetHelper : public FacetHelper
{
public:
	std::pair<unsigned , std::string> generateIDAndName(const Score & attributeValue) ;
	void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) ;
	void initialize(const std::string * info , const Schema * schema) ;

	std::map<std::string, unsigned> categoryValueToBucketIdMap;
};

class RangeFacetHelper : public FacetHelper
{
public:
	RangeFacetHelper(){
		generateListOfIdsAndNamesFlag = false;
	}
	std::pair<unsigned , std::string> generateIDAndName(const Score & attributeValue) ;
	void generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames) ;
	void initialize(const std::string * info , const Schema * schema) ;

	Score start, end, gap;
	unsigned numberOfBuckets;
	bool generateListOfIdsAndNamesFlag;
};

////////////////////////////////////////////////////////////////////////////////
// FacetResultsContainer and its two children
class FacetResultsContainer
{
public:
	virtual void initialize(FacetHelper * facetHelper , FacetAggregationType  opCode) = 0;
	virtual void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType opCode) = 0;
	virtual void getNamesAndValues(std::vector<std::pair< std::string, float > > & results) = 0;
	virtual ~FacetResultsContainer() {};

	float initAggregation(FacetAggregationType  opCode);
	float doAggregation(float bucketValue, FacetAggregationType  opCode);
};

class CategoricalFacetResultsContainer : public FacetResultsContainer
{
public:
	// a map from bucket id to the pair of bucketname,bucketvalue
	std::map<unsigned, std::pair<std::string, float> > bucketsInfo;

	void initialize(FacetHelper * facetHelper , FacetAggregationType  opCode);
	void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType opCode);
	void getNamesAndValues(std::vector<std::pair< std::string, float > > & results);

};

class RangeFacetResultsContainer : public FacetResultsContainer
{
public:
	// A list of pairs of <bucketname,bucketvalue> that bucketId is the index of each bucket in this list
	std::vector<std::pair<std::string, float> > bucketsInfo;

	void initialize(FacetHelper * facetHelper , FacetAggregationType  opCode);
	void addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType opCode);
	void getNamesAndValues(std::vector<std::pair< std::string, float > > & results);

};

class FacetedSearchFilterInternal
{

public:
    FacetedSearchFilterInternal(){
    }

    ~FacetedSearchFilterInternal(){
    	for(std::map<std::string , FacetHelper *>::iterator facetHelperPtr = facetHelpers.begin() ;
    			facetHelperPtr != facetHelpers.end() ; ++facetHelperPtr){
    		if(facetHelperPtr->second != NULL){
    			delete facetHelperPtr->second;
    		}
    	}
    	for(std::map<std::string , std::pair< FacetType , FacetResultsContainer * > >::iterator facetResultsPtr = facetResults.begin();
    			facetResultsPtr != facetResults.end(); ++facetResultsPtr){
    		if(facetResultsPtr->second.second != NULL){
    			delete facetResultsPtr->second.second;
    		}
    	}
    }

    void doFilter(IndexSearcher *indexSearcher,
            const Query * query, QueryResults * input, QueryResults * output) ;
    void preFilter(IndexSearcher *indexSearcher);
    void doProcessOneResult(const Score & attributeValue, const std::string & facetFieldName);

    void initialize(std::vector<FacetType> & facetTypes,
            std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
            std::vector<std::string> & rangeEnds,
            std::vector<std::string> & rangeGaps);

    // TODO : change fields from string to unsigned (attribute IDs)
    std::vector<FacetType> facetTypes;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;

    // a map between facet field names and facet helper pointers
    std::map<std::string , FacetHelper *> facetHelpers;
	std::map<std::string , std::pair< FacetType , FacetResultsContainer * > > facetResults;
};

}
}

#endif // __CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H__
