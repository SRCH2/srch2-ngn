


#include "FacetOperator.h"
#include "PhysicalOperatorsHelper.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "util/Assert.h"
#include "instantsearch/TypedValue.h"
#include "util/DateAndTimeHandler.h"
#include "operation/QueryEvaluatorInternal.h"
#include "util/RecordSerializerUtil.h"
using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

bool compareFacetGroups(const std::pair<std::string, float> & first , const std::pair<std::string, float> & second){
	return first.second > second.second ;
}

float FacetResultsContainer::initAggregation(FacetAggregationType  aggregationType){
	switch (aggregationType) {
		case FacetAggregationTypeCount:
			return 0;
		default:
			ASSERT(false);
			return 0;
	}
}
float FacetResultsContainer::doAggregation(float bucketValue, FacetAggregationType  aggregationType){
	switch (aggregationType) {
		case FacetAggregationTypeCount:
			return bucketValue + 1;
		default:
			ASSERT(false);
			return 0;
	}
}

void CategoricalFacetResultsContainer::initialize(FacetHelper * facetHelper , FacetAggregationType  aggregationType){
	// No need to do anything in this function.
}
void CategoricalFacetResultsContainer::addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType aggregationType){

	std::map<unsigned, std::pair<std::string, float> >::iterator bucketIter = bucketsInfo.find(bucketId);
	if( bucketIter == bucketsInfo.end()){ // A new bucket
		std::pair<std::string, float> newBucket = std::make_pair(bucketName , initAggregation(aggregationType));
		newBucket.second = doAggregation(newBucket.second , aggregationType);
		bucketsInfo[bucketId] = newBucket;
	}else{
		bucketIter->second.second = doAggregation(bucketIter->second.second , aggregationType);
	}

}
void CategoricalFacetResultsContainer::getNamesAndValues(std::vector<std::pair< std::string, float > > & results, int numberOfGroupsToReturn ){
	// we use 'results' to copy the data from the map and sort it
	for(std::map<unsigned, std::pair<std::string, float> >::iterator bucketPtr = bucketsInfo.begin() ;
			bucketPtr != bucketsInfo.end() ; ++bucketPtr){
		results.push_back(std::make_pair(bucketPtr->second.first , bucketPtr->second.second));
	}
	// now sort it
	std::sort(results.begin() , results.end() , &compareFacetGroups);
	// if the query wants all the groups or the available groups are fewer than what the query asks for, we don't need to remove the tail
	if(numberOfGroupsToReturn < 0 || numberOfGroupsToReturn > results.size()){
		return;
	}
	// remove the tail
	std::vector<std::pair< std::string, float > >::iterator startOfTailToRemove = results.begin();
	startOfTailToRemove += numberOfGroupsToReturn;
	results.erase(startOfTailToRemove , results.end());
}


/*
 * Since we want to include all intervals of a range facet (even if they don't have any records in them)
 * we have to first initialize all intervals to a value (e.g. count of 0). To do this we use facetHelper to give us
 * a list of IDs which it will generate later with their name. For example:
 * <0,"-inf">,<1,"10">,<2,"20">,<3,"30">
 */
void RangeFacetResultsContainer::initialize(FacetHelper * facetHelper , FacetAggregationType  aggregationType){
	std::vector<std::pair<unsigned, std::string> > idsAndNames;
	facetHelper->generateListOfIdsAndNames(&idsAndNames);
	int index = 0;
	for(std::vector<std::pair<unsigned, std::string> >::iterator idAndName = idsAndNames.begin();
			idAndName != idsAndNames.end() ; ++idAndName){
		ASSERT(index == idAndName->first); // bucket Id must be the index of buckets in this vector
		bucketsInfo.push_back(std::make_pair(idAndName->second , initAggregation(aggregationType) ));
		//
		index++;
	}
}
void RangeFacetResultsContainer::addResultToBucket(const unsigned bucketId, const std::string & bucketName, FacetAggregationType aggregationType){
	bucketsInfo.at(bucketId).second = doAggregation(bucketsInfo.at(bucketId).second , aggregationType);
}
void RangeFacetResultsContainer::getNamesAndValues(std::vector<std::pair< std::string, float > > & results, int numberOfGroupsToReturn){
	results.insert(results.begin(), bucketsInfo.begin() , bucketsInfo.end());
}


std::pair<unsigned , std::string> CategoricalFacetHelper::generateIDAndName(const TypedValue & attributeValue){
	std::string attributeValueLowerCase = attributeValue.toString();
	std::transform(attributeValueLowerCase.begin(), attributeValueLowerCase.end(), attributeValueLowerCase.begin(), ::tolower);

	if(categoryValueToBucketIdMap.find(attributeValueLowerCase) == categoryValueToBucketIdMap.end()){
		categoryValueToBucketIdMap[attributeValueLowerCase] = categoryValueToBucketIdMap.size();
	}
	return std::make_pair(categoryValueToBucketIdMap[attributeValueLowerCase] , attributeValueLowerCase);
}


void FacetHelper::generateIDAndNameForMultiValued(const TypedValue & attributeValue ,
		std::vector< std::pair<unsigned , std::string> > & resultIdsAndNames){
	ASSERT(attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_UNSIGNED ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_FLOAT ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_TEXT ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_TIME);
	std::vector<TypedValue> singleValues;
	attributeValue.breakMultiValueIntoSingleValueTypedValueObjects(&singleValues);
	for(std::vector<TypedValue>::iterator singleValue = singleValues.begin() ; singleValue != singleValues.end() ; ++singleValue){
		std::pair<unsigned, std::string>  idAndNamePair = generateIDAndName(*singleValue);
		if(std::find(resultIdsAndNames.begin() , resultIdsAndNames.end() , idAndNamePair) == resultIdsAndNames.end()){
			resultIdsAndNames.push_back(idAndNamePair);
		}
	}
}

void CategoricalFacetHelper::generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames){
	// This function should not be called.
	ASSERT(false);
}
void CategoricalFacetHelper::initialize(const std::string * facetInfoForInitialization, const Schema * schema){
	// No need to do anything here
}

/*
 * This function finds the interval in which attributeValue is placed. ID and name will be returned. since all the names are returned once
 * in generateListOfIdsAndNames, all names are returned as "" to save copying string values.
 */
std::pair<unsigned , std::string> RangeFacetHelper::generateIDAndName(const TypedValue & attributeValue){
	if(attributeValue >= end){
		return std::make_pair(numberOfBuckets-1 , "");
	}
	unsigned bucketId = attributeValue.findIndexOfContainingInterval(start, end, gap);
    if(bucketId == -1){ // Something has gone wrong and Score class has been unable to calculate the index.
    	ASSERT(false);
        return std::make_pair(0 , "");;
    }
    if(bucketId >= numberOfBuckets){
    	bucketId =  numberOfBuckets - 1;
    }
    return std::make_pair(bucketId , "");
}

/*
 * Example :
 * If we have two attributes : price,model (facet type : range, categorical)
 * and start,end and gap for price are 1,100 and 10. Then this function
 * produces an empty vector for model (because it's categorical) and a vector with the following
 * values for price:
 * -large_value, 1, 11, 21, 31, 41, ..., 91, 101
 */
void RangeFacetHelper::generateListOfIdsAndNames(std::vector<std::pair<unsigned, std::string> > * idsAndNames){

	if(generateListOfIdsAndNamesFlag == true || idsAndNames == NULL){
		ASSERT(false);
		return;
	}
	TypedValue lowerBoundToAdd = start;
    std::vector<TypedValue> lowerBounds;
	// Example : start : 1, gap : 10 , end : 100
	// first -large_value is added as the first category
	// then 1, 11, 21, ...and 91 are added in the loop.
	// and 101 is added after loop.
	lowerBounds.push_back(lowerBoundToAdd.minimumValue()); // to collect data smaller than start
	while (lowerBoundToAdd < end) {
		lowerBounds.push_back(lowerBoundToAdd); // data of normal categories
		lowerBoundToAdd = lowerBoundToAdd + gap;
	}
	lowerBounds.push_back(end); // to collect data greater than end
	// now set the number of buckets
	numberOfBuckets = lowerBounds.size();
	// now fill the output
	for(int lowerBoundsIndex = 0; lowerBoundsIndex < lowerBounds.size() ; lowerBoundsIndex++){
		string bucketName = lowerBounds.at(lowerBoundsIndex).toString();
		/*
		 * If the type of this facet attribute is time, we should translate the number of
		 * seconds from Jan 1st 1970 (aka "epoch") to a human readable representation of time.
		 * For example, if this value is 1381271294, the name of this bucket is 10/8/2013 3:28:14.
		 */
		if(attributeType == ATTRIBUTE_TYPE_TIME){
			long timeValue = lowerBounds.at(lowerBoundsIndex).getTimeTypedValue();
			bucketName = DateAndTimeHandler::convertSecondsFromEpochToDateTimeString(&timeValue);
		}
		idsAndNames->push_back(std::make_pair(lowerBoundsIndex , bucketName));
	}
	// And set the flag to make sure this function is called only once.
	generateListOfIdsAndNamesFlag = false;

}

/*
 * Info must contain :
 * facetInfoForInitialization[0] <= start
 * facetInfoForInitialization[1] <= end
 * facetInfoForInitialization[2] <= gap
 * facetInfoForInitialization[3] <= fieldName
 */
void RangeFacetHelper::initialize(const std::string * facetInfoForInitialization , const Schema * schema){
	std::string startString = facetInfoForInitialization[0];
	std::string endString = facetInfoForInitialization[1];
	std::string gapString = facetInfoForInitialization[2];
	std::string fieldName = facetInfoForInitialization[3];

    attributeType = schema->getTypeOfRefiningAttribute(
            schema->getRefiningAttributeId(fieldName));
    start.setTypedValue(attributeType, startString);

    end.setTypedValue(attributeType, endString);

    if(attributeType == ATTRIBUTE_TYPE_TIME){
    	// For time attributes gap should not be of the same type, it should be
    	// of type TimeDuration.
    	if(start > end){ // start should not be greater than end
    		start = end;
    		gap.setTypedValue(ATTRIBUTE_TYPE_DURATION, "00:00:00");
    	}else{
    		gap.setTypedValue(ATTRIBUTE_TYPE_DURATION, gapString);
    	}
    }else{
    	if(start > end){ // start should not be greater than end
    		start = end;
    		gap.setTypedValue(attributeType , "0");
    	}else{
    		gap.setTypedValue(attributeType, gapString);
    	}
    }

}


bool FacetOperator::open(QueryEvaluatorInternal * queryEvaluatorInternal, PhysicalPlanExecutionParameters & params){

	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);

    Schema * schema = this->queryEvaluatorInternal->getSchema();
	// first prepare internal structures based on the input
    preFilter(this->queryEvaluatorInternal);

    this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(this->queryEvaluatorInternal,params);
    return true;
}

/*
 * Each time getNext is called, the next record, if any, is retrieved from the child and
 * the facet info is updated by using the refining attribute value from forward index.
 * That record is returned to the user and facet info is retrieved from facet at the end by the caller.
 */
PhysicalPlanRecordItem * FacetOperator::getNext(const PhysicalPlanExecutionParameters & params) {

    Schema * schema = this->queryEvaluatorInternal->getSchema();
    ForwardIndex * forwardIndex = this->queryEvaluatorInternal->getForwardIndex();
	// move on the results once and do all facet calculations.
    shared_ptr<vectorview<ForwardListPtr> > readView;
    this->queryEvaluatorInternal->getForwardIndex_ReadView(readView);

    const ForwardList *forwardList;
    PhysicalPlanRecordItem *resultIter;
    // loop to find the next valid record (i.e., not deleted)
    while (true) {
      resultIter = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->getNext(params);
      if(resultIter == NULL){
	return NULL;
      }
      // extract all facet related refining attribute values from this record
      // by accessing the forward index only once.
      bool isValid = false;
      forwardList = forwardIndex->getForwardList(readView, resultIter->getRecordId() , isValid);
      if (isValid) // found a valid one; otherwise, continue
	break;
    }

	StoredRecordBuffer refiningAttributesData =
			forwardList->getInMemoryData();
	// this vector is parallel to attributeIds vector
	std::vector<TypedValue> attributeDataValues;
	RecordSerializerUtil::getBatchOfAttributes(fields, schema,refiningAttributesData.start.get(), &attributeDataValues);

	// now iterate on attributes and incrementally update the facet results
	for(std::vector<std::string>::iterator facetField = fields.begin();
			facetField != fields.end() ; ++facetField){
		TypedValue & attributeValue = attributeDataValues.at(
							   std::distance(fields.begin() , facetField));
		// choose the type of aggregation for this attribute
		// increments the correct facet by one
		doProcessOneResult(attributeValue , std::distance(fields.begin() , facetField));
	}
	return resultIter;

}
bool FacetOperator::close(PhysicalPlanExecutionParameters & params){
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
	this->facetTypes.clear();
	this->fields.clear();
	this->rangeStarts.clear();
	this->rangeEnds.clear();
	this->rangeGaps.clear();
	this->numberOfGroupsToReturnVector.clear();
	this->facetHelpers.clear();
	this->facetResults.clear();


	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

//As of now, cache implementation doesn't need this function for this operator.
// This code is here only if we want to implement a
//cache module in future that needs it.
string FacetOperator::toString(){
	ASSERT(false); // this function should not be called for this operator
	// because we don't have any cache functionality for facet as of now.
	string result = "facetOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}
bool FacetOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {
	ASSERT(false);
	return false;
}
void FacetOperator::getFacetResults(QueryResults * output){

    // now copy all results to output
	for(std::vector<std::pair< FacetType , FacetResultsContainer * > >::iterator facetResultsPtr = facetResults.begin();
			facetResultsPtr != facetResults.end(); ++facetResultsPtr){
		std::vector<std::pair< std::string, float > > results;
		int numberOfGroupsToReturnForThisField = -1;
		// We make sure that numberOfGroupsToReturnVector has the same size as the number of facet attributes.
		// If it doesn't have the same size (input to this function is not consistent), we use -1 and don't remove the tail.
		if(this->numberOfGroupsToReturnVector.size() == fields.size()){
			numberOfGroupsToReturnForThisField = this->numberOfGroupsToReturnVector.at(std::distance(facetResults.begin() , facetResultsPtr));
		}
		facetResultsPtr->second->getNamesAndValues(results, numberOfGroupsToReturnForThisField);
		output->impl->facetResults[fields.at(std::distance(facetResults.begin() , facetResultsPtr))] = std::make_pair(facetResultsPtr->first , results);
	}
}

FacetOperator::~FacetOperator(){
	// it must close before deletion
	ASSERT(facetHelpers.size() == 0);
}
FacetOperator::FacetOperator(QueryEvaluatorInternal *queryEvaluatorInternal, std::vector<FacetType> & facetTypes,
        std::vector<std::string> & fields, std::vector<std::string> & rangeStarts,
        std::vector<std::string> & rangeEnds,
        std::vector<std::string> & rangeGaps, std::vector<int> & numberOfGroupsToReturn) {
	this->queryEvaluatorInternal = queryEvaluatorInternal;
    this->fields = fields;
    this->facetTypes = facetTypes;
    this->rangeStarts = rangeStarts;
    this->rangeEnds = rangeEnds;
    this->rangeGaps = rangeGaps;
    this->numberOfGroupsToReturnVector = numberOfGroupsToReturn;
}



void FacetOperator::preFilter(QueryEvaluatorInternal *queryEvaluatorInternal){

    Schema * schema = queryEvaluatorInternal->getSchema();

	for(std::vector<std::string>::iterator facetField = fields.begin();
            facetField != fields.end() ; ++facetField){
		FacetType facetType = facetTypes.at(std::distance(fields.begin() , facetField));
		FacetHelper * facetHelper = NULL;
		FacetResultsContainer * facetResultsContainer = NULL;
		switch (facetType) {
			case FacetTypeCategorical:
				facetHelper = new CategoricalFacetHelper();
				facetResultsContainer = new CategoricalFacetResultsContainer();
				break;
			case FacetTypeRange:
				facetHelper = new RangeFacetHelper();
				facetResultsContainer = new RangeFacetResultsContainer();
				break;
			default:
				ASSERT(false);
				break;
		}
		std::string info[4];
		info[0] = rangeStarts.at(std::distance(fields.begin() , facetField));
		info[1] = rangeEnds.at(std::distance(fields.begin() , facetField));
		info[2] = rangeGaps.at(std::distance(fields.begin() , facetField));
		info[3] = *facetField;
		facetHelper->initialize(info , schema);
		facetResultsContainer->initialize(facetHelper , FacetAggregationTypeCount );
		this->facetResults.push_back(std::make_pair(facetType , facetResultsContainer));
		this->facetHelpers.push_back(facetHelper);

	}
}
void FacetOperator::doProcessOneResult(const TypedValue & attributeValue, const unsigned facetFieldIndex){
	if(attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_UNSIGNED ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_FLOAT ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_TEXT ||
			attributeValue.getType() == ATTRIBUTE_TYPE_MULTI_TIME){
		std::vector<std::pair<unsigned , std::string> > idsAndNames;
		this->facetHelpers.at(facetFieldIndex)->generateIDAndNameForMultiValued(attributeValue , idsAndNames);
		for(std::vector<std::pair<unsigned , std::string> >::iterator idAndName = idsAndNames.begin() ; idAndName != idsAndNames.end() ; ++idAndName){
			this->facetResults.at(facetFieldIndex).second->addResultToBucket(idAndName->first , idAndName->second , FacetAggregationTypeCount);
		}
	}else{ // single value
		std::pair<unsigned , std::string> idandName = this->facetHelpers.at(facetFieldIndex)->generateIDAndName(attributeValue);
		this->facetResults.at(facetFieldIndex).second->addResultToBucket(idandName.first , idandName.second , FacetAggregationTypeCount);
	}
}


// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost FacetOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost FacetOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost FacetOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(1);
}
PhysicalPlanCost FacetOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	return PhysicalPlanCost(1);
}
void FacetOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	return;
}
void FacetOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	return;
}
PhysicalPlanNodeType FacetOptimizationOperator::getType() {
	return PhysicalPlanNode_Facet;
}
bool FacetOptimizationOperator::validateChildren(){
	return true;
}




}
}
