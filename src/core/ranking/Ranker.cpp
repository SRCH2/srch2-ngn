
// $Id: Ranker.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include <instantsearch/Ranker.h>
#include "util/Assert.h"
#include <iostream>
#include <math.h>
#include "util/AttributeIterator.h"
#include <cfloat>

using std::vector;

namespace srch2
{
    namespace instantsearch
    {
    // text relevance score computed during indexing time and stored in the index
    float DefaultTopKRanker::computeRecordTfIdfScore(const float tf, const float idf, const float sumOfFieldBoosts)
    {
        return tf *idf * sumOfFieldBoosts;
    }
    
    // this function is used in the term virtual list (heap) to compute the score of
    // a record with respect to a keyword 
    float DefaultTopKRanker::computeTermRecordRuntimeScore(float termRecordStaticScore, 
                                   unsigned editDistance, unsigned termLength, 
                                   bool isPrefixMatch,
                                   float prefixMatchPenalty , float termSimilarityBoost)
    {
        unsigned ed = editDistance;
        if (ed > termLength)
        	ed = termLength;

        // TODO : change this power calculation to using a pre-computed array
        float normalizedEdSimilarity = (1 - (1.0*ed) / termLength)* pow(termSimilarityBoost, (float) ed);
        float PrefixMatchingNormalizer = isPrefixMatch ? (prefixMatchPenalty) : 1.0;
        return termRecordStaticScore * normalizedEdSimilarity * PrefixMatchingNormalizer;
    }
    
    float DefaultTopKRanker::computeOverallRecordScore(const Query *query, 
                               const vector<float> &queryResultTermScores) const
    {
        const vector<Term *> *queryTerms = query->getQueryTerms();
        ASSERT(queryTerms->size() == queryResultTermScores.size());
        
        float overallScore = 0.0;
        vector<float>::const_iterator queryResultsIterator = queryResultTermScores.begin();
        for (vector<Term *>::const_iterator queryTermsIterator = queryTerms->begin(); 
         queryTermsIterator != queryTerms->end(); queryTermsIterator++ , queryResultsIterator++ ) {
            float termRecordRuntimeScore =  (*queryResultsIterator);
            // apply the aggregation function
            overallScore = DefaultTopKRanker::aggregateBoostedTermRuntimeScore(overallScore,
                                           (*queryTermsIterator)->getBoost(),
                                           termRecordRuntimeScore);
        }

        return overallScore;
    }

    // aggregation function used in top-k queries (Fagin's algorithm)
    float DefaultTopKRanker::aggregateBoostedTermRuntimeScore(float oldRecordScore,
                                  float termBoost,
                                  float termRecordRuntimeScore) const
    {
        // We use the "addition" function to do the aggregation
        float newRecordScore = oldRecordScore + termBoost * termRecordRuntimeScore;
        return newRecordScore;
    }

    bool DefaultTopKRanker::compareRecordsLessThan(float leftRecordScore,  unsigned leftRecordId,
                               float rightRecordScore, unsigned rightRecordId)
    {
        if (leftRecordScore == rightRecordScore)
        return leftRecordId > rightRecordId;  // earlier records are ranked higher
        else
        return leftRecordScore < rightRecordScore;
    }

    bool DefaultTopKRanker::compareRecordsGreaterThan(float leftRecordScore,  unsigned leftRecordId,
                              float rightRecordScore, unsigned rightRecordId)
    {
        if (leftRecordScore == rightRecordScore)
        return leftRecordId < rightRecordId; 
        else
        return leftRecordScore > rightRecordScore;
    }

    //TODO
    bool DefaultTopKRanker::compareRecordsGreaterThan(const TypedValue & leftRecordTypedValue,  unsigned leftRecordId,
    		const TypedValue & rightRecordTypedValue, unsigned rightRecordId)
    {
    	if (leftRecordTypedValue == rightRecordTypedValue)
    		return leftRecordId < rightRecordId;
    	else
    		return leftRecordTypedValue > rightRecordTypedValue;
    }


    float DefaultTopKRanker::computeAggregatedRuntimeScoreForAnd(std::vector<float> runTimeTermRecordScores){

    	float resultScore = 0;

    	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
    		resultScore += *(score);
    	}
    	return resultScore;
    }

    float DefaultTopKRanker::computeAggregatedRuntimeScoreForOr(std::vector<float> runTimeTermRecordScores){

    	// max
    	float resultScore = -1;

    	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
    		if((*score) > resultScore){
    			resultScore = (*score);
    		}
    	}
    	return resultScore;
    }

    float DefaultTopKRanker::computeScoreForNot(float score){
    	return 1 - score;
    }

    /*float DefaultTopKRanker::computeOverallRecordScore(const Query *query, const vector<float> &queryResultTermScores, unsigned recordLength) const
      {
      const vector<Term *> *queryTerms = query->getQueryTerms();
      
      ASSERT(queryTerms->size() == queryResultTermScores.size());
      
      float overallScore = 0.0;
      vector<float>::const_iterator queryResultsIterator = queryResultTermScores.begin();
      for ( vector<Term *>::const_iterator queryTermsIterator = queryTerms->begin(); queryTermsIterator != queryTerms->end(); queryTermsIterator++ , queryResultsIterator++ )
    {
    overallScore += computeTermWeightedRecordScore(*queryTermsIterator, *queryResultsIterator);
    }
    
    // we add the boost value based on the record length
    overallScore = overallScore + ( (1 / (float)(recordLength + 4)) * query->getLengthBoost());
    
    return overallScore;
    }*/

//TODO: BUG: Make length const after fixing bug ticket 36
/*float DefaultTopKRanker::computeSimilarity(const Term *term, const float invertedListElementScore, const unsigned distance, const unsigned length) const
{
    float returnValue;
    switch(distance)
    {
        case 0:
               returnValue = ((invertedListElementScore ))/(float)(length+1);
               break;
        case 1:
            returnValue = (invertedListElementScore * term->getSimilarityBoost())/(float)(4*(length+1));
            break;
        case 2:
            returnValue = (invertedListElementScore * term->getSimilarityBoost())/(float)(16*(length+1));
            break;
        case 3:
            returnValue = (invertedListElementScore * term->getSimilarityBoost())/(float)(32*(length+1));
            break;
        default:
            returnValue = (invertedListElementScore * term->getSimilarityBoost())/(float)(64*(length+1));
            break;
     }
    return returnValue;

    float returnValue = 0.0;
    //float fuzzySimilarityScore = (1-((float)distance/(float)length));
    float fuzzySimilarityScore = (1-( (float)distance/(float)(term->getKeyword()->length()) ));
    float w = term->getSimilarityBoost();
    returnValue = ( w * fuzzySimilarityScore ) + ( (1 - w) * invertedListElementScore);
    return returnValue;
}*/


float GetAllResultsRanker::computeResultScoreUsingAttributeScore(const Query *query, const float recordAttributeScore, const unsigned totalEditDistance, const unsigned totalQueryLength) const
{
    const vector<Term *> *queryTerms = query->getQueryTerms();
    float returnValue = 0.0;
    float fuzzySimilarityScore = (1-((float)totalEditDistance/(float)totalQueryLength));
    
    for ( vector<Term *>::const_iterator queryTermsIterator = queryTerms->begin(); 
      queryTermsIterator != queryTerms->end(); queryTermsIterator++ ) {
    const Term *term = *queryTermsIterator;
    float w = term->getSimilarityBoost();
    returnValue += ( w * fuzzySimilarityScore ) + ( (1 - w) * recordAttributeScore );
    break;
    }
    return returnValue;
}

double SpatialRanker::getDistanceRatio(const double minDist2UpperBound, const double resultMinDist2 ) const
{
    return ((double)sqrt(minDist2UpperBound) - (double)sqrt(resultMinDist2)) / (double)sqrt(minDist2UpperBound);
}

// uses the function in Geo
// two changes are made because we use range query here instead of kNN query
//    1. the "query point" is set to the center point of the query range
//    2. instead of using a fixed minDist2UpperBound, we need to set it according to the range's radius
//     the "search radius" is set to half of the diagonal
float SpatialRanker::combineKeywordScoreWithDistanceScore(const float keywordScore, const float distanceScore) const
{
    return distanceScore * keywordScore;
}

double SpatialRanker::calculateHaversineDistanceBetweenTwoCoordinates(double latitude1, double longitude1, double latitude2, double longitude2) const
{
    double radius = 6371.0; // radius of earth in kms
    double dLat = degreeToRadian(latitude2-latitude1);
    double dLon = degreeToRadian(longitude2-longitude1);
    double lat1 = degreeToRadian(latitude1);
    double lat2 = degreeToRadian(latitude2);
    double alpha = sin(dLat/2.0) * sin(dLat/2.0) + sin(dLon/2.0) * sin(dLon/2.0) * cos(lat1) * cos(lat2);
    double angularDistance = 2.0 * atan2(sqrt(alpha), sqrt(1.0 - alpha));
    double distanceInKilometers = radius * angularDistance;
    double distanceInMiles = distanceInKilometers * 0.6214;

    return distanceInMiles;
}

// Convert degree value to radian value using the formula : radian = degree*PI/180
double SpatialRanker::degreeToRadian(double degreeValue) const
{
    const double PI = 3.1415926535;
    return degreeValue * PI / 180.0;
}

float DynamicScoringRanker::CalculateDynamicKeywordScore(
    const KeywordBoost& keyword, DynamicScoringFilter& dynamicScoringFilter) {
  if(keyword.score == 0) return 0;

  float boostValue= 0;
  
  /* Loops over all boosted attributes containing this keyword */
  for(AttributeIterator attribute(keyword.attributeMask);
      attribute.hasNext(); ++attribute) {
    const AttributeBoost& attributeBoost= 
      *dynamicScoringFilter.getAttributeBoost(*attribute);
    /* Each attribute boost is the log base e of the number of keyword hits in
       that attribute plus e-1, ensuring the log is greater than 1, multiplied
       by the attribute's boosting factor */ 
    boostValue+= 
        std::log(attributeBoost.hitCount-1+M_E) * attributeBoost.boostFactor;
  }

  return boostValue * keyword.score; 
}
 

float DynamicScoringRanker::CalculateAndAggregrateDynamicScore(
    const KeywordBoost* keyword, unsigned numberOfKeywords,
    DynamicScoringFilter& dynamicScoringFilter) {
  float score= 0;
  float boostValue=1;
  AttributeBoost *attributeBoost;

  for(unsigned i=0; i < numberOfKeywords; ++i, ++keyword) {
    score+= CalculateDynamicKeywordScore(*keyword, dynamicScoringFilter); 
  }
  return score;
}

uint8_t computeEditDistanceThreshold(unsigned keywordLength , float similarityThreshold)
{
	if(similarityThreshold < 0 || similarityThreshold > 1) {
		ASSERT(false);
		return 0;
	}
	// We add "FLT_EPSILON" to deal with imprecise representations of float.
	float fresult = keywordLength * (1 - similarityThreshold + FLT_EPSILON);
	return fresult; // casting to unsigned int will do the floor operation automatically.
}

}}
