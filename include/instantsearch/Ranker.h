// $Id: Ranker.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#ifndef __RANKER_H__
#define __RANKER_H__

#include <instantsearch/Constants.h>
#include <instantsearch/Term.h>
#include <instantsearch/Query.h>
#include <instantsearch/TypedValue.h>
#include <vector>

using std::vector;

namespace srch2
{
    namespace instantsearch
    {
    // TODO: Copy comments from wiki page and expand it for each function
    class Ranker
    {
    public:
        // tf-idf score computed during index construction phase
        static float computeRecordTfIdfScore(const float tf, const float idf, const float sumOfFieldBoosts);
        static float computeTermRecordRuntimeScore(float recordStaticScore, 
                               unsigned editDistance, unsigned termLength, 
                               bool isPrefixMatch, float prefixMatchPenalty, float termSimilarityBoost);

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



        virtual ~Ranker() {};
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
    
    }
}

#endif /* __RANKER_H__ */
