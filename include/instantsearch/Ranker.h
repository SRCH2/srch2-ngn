
#ifndef __RANKER_H__
#define __RANKER_H__

#include <instantsearch/Constants.h>
#include <instantsearch/Term.h>
#include <instantsearch/Query.h>
#include <instantsearch/TypedValue.h>
#include "record/LocationRecordUtil.h"
#include <vector>

using std::vector;

namespace srch2 {
  namespace instantsearch {
    // TODO: Copy comments from wiki page and expand it for each function
    class Ranker
    {
    public:
        // tf-idf score computed during index construction phase
        static float computeTextRelevance(const float tfAndSumOfFieldBoosts, const float idf);
        static float computeRecordTfBoostProdcut(const float tf, const float sumOfFieldBoosts);
        static float computeTermRecordRuntimeScore(float recordStaticScore, 
                               unsigned editDistance, unsigned termLength, 
                               bool isPrefixMatch, float prefixMatchPenalty, float termSimilarityBoost);

        static float computeIdf(unsigned totalDocumentsCount, unsigned termHitDocumentsCount) {
        	return 1 + log (totalDocumentsCount / ((float)(termHitDocumentsCount)+1) );
        }

        static double computeScoreforGeo(Point &recordPosition, Shape &queryShape);

        virtual float aggregateBoostedTermRuntimeScore(float oldRecordScore, 
                               float termBoost,
                               float termRecordRuntimeScore) const;

        virtual float computeOverallRecordScore(const Query *query, const vector<float> &queryResultTermScores) const;

        // compare two records based on their scores and record ids ("<")
        // used in TermVirtualList.h as a "<" operator to rank items on the min-heap
        static bool compareRecordsLessThan(float leftRecordScore, unsigned leftRecordId,
                           float rightRecordScore, unsigned rightRecordId);

        // compare two records based on their scores and record ids
        // used in InvertedList and QueryResultsInternal as a ">" operator to rank the elements 
        // on a list in a descending order
        static bool compareRecordsGreaterThan(float leftRecordScore,  unsigned leftRecordId,
                          float rightRecordScore, unsigned rightRecordId);
        // compare two records based on their scores and record ids
        // used in InvertedList and QueryResultsInternal as a ">" operator to rank the elements
        // on a list in a descending order
        static bool compareRecordsGreaterThan(const TypedValue &  leftRecordScore,  unsigned leftRecordId,
                          const TypedValue & rightRecordScore, unsigned rightRecordId);//TODO

        // Used by GetAllResultsRanker. Default implementation returns 0.
        virtual float computeResultScoreUsingAttributeScore(const Query *query, 
                                const float recordAttributeScore, 
                                const unsigned totalEditDistance, 
                                const unsigned totalQueryLength) const
        {
        return 0.0;
        }
        
        virtual float computeAggregatedRuntimeScoreForAnd(std::vector<float> runTimeTermRecordScores);

        virtual float computeAggregatedRuntimeScoreForOr(std::vector<float> runTimeTermRecordScores);

        virtual float computeScoreForNot(float score);

        //computes sloppy frequency of a phrase based on slop distances
        float computeSloppyFrequency(const vector<unsigned>& listOfSlopDistances) const;
        //computes run-time score for phrase operator
        float computePositionalScore(float runtimeScore, float sloppyFrequency) const;

        virtual ~Ranker() {};

        static float computeFeedbackBoost(unsigned feedbackRecencyInSecs, unsigned feedbackFrequency);
        static float computeFeedbackBoostedScore(float score, float boost);
    };
    
    typedef Ranker DefaultTopKRanker;

    class GetAllResultsRanker : public Ranker
    {
    public:
        float computeResultScoreUsingAttributeScore(const Query *query, const float recordAttributeScore, 
                            const unsigned totalEditDistance, 
                            const unsigned totalQueryLength) const;
    };
    
    class SpatialRanker: public Ranker
    {
    public:
        virtual double getDistanceRatio(const double minDist2UpperBound, const double resultMinDist2 ) const;
        
        // uses the function in M0
        // two changes are made because we use range query here instead of kNN query
        //    1. the "query point" is set to the center point of the query range
        //    2. instead of using a fixed minDist2UpperBound, we need to set it according to the range's radius
        //       the "search radius" is set to half of the diagonal
        virtual float combineKeywordScoreWithDistanceScore(const float keywordScore, const float distanceScore) const;
        
        virtual double calculateHaversineDistanceBetweenTwoCoordinates(double latitude1, double longitude1, 
                                       double latitude2, double longitude2) const;

        // Convert degree value to radian value using the formula : radian = degree*PI/180
        virtual double degreeToRadian(double degreeValue) const;
    };

    /*
     * This function computes the edit-distance, with adjustments for floating point conversion error (epsilon),
     * based on the length of the keyword and a normalizationFactor which must be between 0 and 1.
     * 0 means smaller edit-distance (0) and 1 means larger edit-distance (length of keyword).
     */
    uint8_t computeEditDistanceThreshold(unsigned keywordLength , float similarityThreshold);
    }
}


#endif /* __RANKER_H__ */
