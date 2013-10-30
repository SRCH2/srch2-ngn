//$Id: Scoring_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#define MAX_QUERY_NUMBER 5000

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

Indexer *buildIndex(string data_file, string index_dir, string expression)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;

        while(getline(lineStream,cell,'^') && cellCounter < 4 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }

            cellCounter++;
        }

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

Indexer *buildGeoIndex(string data_file, string index_dir, string expression)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);
    schema->setScoringExpression(expression);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2is::DISABLE_STEMMER_NORMALIZER,
                    "", "","", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2is::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;
        float lat=0.0, lng=0.0;

        while(getline(lineStream,cell,'^') && cellCounter < 6 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                lat = atof(cell.c_str());
            }
            else if (cellCounter == 5)
            {
                lng = atof(cell.c_str());
            }

            cellCounter++;
        }

        record->setLocationAttributeValue(lat, lng);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    data.close();

    return indexer;
}

void validateDefaultIndexScoresExpression1(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    // Targeted Record:
    //    name - "Fargen Matthew MD"
    //    category - "Health & Medicine Physicians"
    //    record boost - 6.38022916069
    
    // Ranking Expression: idf_score * doc_boost

    // test 1: exact search, complete match, single term
    //         query: "fargen"
    //         function: idf_score = sumOfFieldBoost * idf
    //                             = (1 + 2/3) * (1+ln(1000/(2+1)))
    //                             = 11.34857165
    //                   total_score = idf_score * doc_boost
    //                               = 72.40648777351
    float score1 = pingToGetTopScore(analyzer, indexSearcher, "fargen");
    cout << "Score: " << score1 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score1 > 72.4065-0.1 && score1 < 72.4065+0.1);
    // test 2: exact search, prefix match, single term
    //         query: "farge"
    //         function: idf_score = sumOfFieldBoost * idf
    //                             = (1 + 2/3) * (1+ln(1000/(2+1)))
    //                             = 11.34857165
    //                   prefixMatchPenalty = 0.95
    //                   total_score = idf_score * doc_boost * prefixMatchPenalty
    //                               = 68.78616338483
    float score2 = pingToGetTopScore(analyzer, indexSearcher, "farge");
    cout << "Score: " << score2 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score2 > 68.7862-0.1 && score2 < 68.7862+0.1);

    // test 3: fuzzy search, complete match, single term
    //         query: "faugen"
    //         function: idf_score = sumOfFieldBoost * idf
    //                             = (1 + 2/3) * (1+ln(1000/(2+1)))
    //                             = 11.34857165
    //                   NormalizedEdSimilarity = 1 - ed/query_term_length
    //                                          = 1 - 1/6 = 5/6
    //                   total_score = idf_score * doc_boost * NormalizedEdSimilarity
    //                               = 60.33873981126
    float score3 = pingToGetTopScore(analyzer, indexSearcher, "faugen");
    cout << "Score: " << score3 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score3 > 30.1302-0.1 && score3 < 30.1302+0.1);

    // test 4: fuzzy search, prefix match, single term
    //         query: "fauge"
    //         function: idf_score = sumOfFieldBoost * idf
    //                             = (1 + 2/3) * (1+ln(1000/(2+1)))
    //                             = 11.34857165
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = 1 - ed/query_term_length
    //                                          = 1 - 1/5 = 4/5
    //                   total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty
    //                               = 55.02893070786
    float score4 = pingToGetTopScore(analyzer, indexSearcher, "fauge");
    cout << "Score: " << score4 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score4 > 27.4787-0.1 && score4 < 27.4787+0.1);

    // test 5: fuzzy search, prefix match, double terms
    //         query: "fauge+medican"
    //         function: Term1:
    //                   idf_score = sumOfFieldBoost * idf
    //                             = (1 + 2/3) * (1+ln(1000/(2+1)))
    //                             = 11.34857165
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 = 4/5
    //                   term1_total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 55.02893070786
    //                   Term2:
    //                   idf_score = sumOfFieldBoost * idf
    //                             = (1 + 1/3) * (1+ln(1000/(118+1))
    //                             = 4.1715090478
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/7 = 6/7
    //                   term2_total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 21.67236384
    //
    //                   total_score = term1_total_score + term2_total_score
    //                               = 76.701294554
    float score5 = pingToGetTopScore(analyzer, indexSearcher, "fauge+medican");
    cout << "Score: " << score5 << endl;
    ASSERT(score5 > 38.3062-0.1 && score5 < 38.3062+0.1);

}

void validateDefaultIndexScoresExpression2(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    // Targeted Record:
    //    name - "Fargen Matthew MD"
    //    category - "Health & Medicine Physicians"
    //    record boost - 6.38022916069
    
    // Ranking Expression: doc_boost + ( 1 / (doc_length+1) )

    // test 1: exact search, complete match, single term
    //         query: "fargen"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   total_score = doc_boost + ( 1 / (doc_length+1) )
    //                               = 6.38022916069 + ( 1 / (6+1) )
    //                               = 6.52308630355
    float score1 = pingToGetTopScore(analyzer, indexSearcher, "fargen");
    cout << "Score: " << score1 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score1 > 6.5230-0.1 && score1 < 6.5231+0.1);

    // test 2: exact search, prefix match, single term
    //         query: "farge"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   prefixMatchPenalty = 0.95
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * prefixMatchPenalty
    //                               = 6.52308630355 * 0.95
    //                               = 6.19693198837
    float score2 = pingToGetTopScore(analyzer, indexSearcher, "farge");
    cout << "Score: " << score2 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score2 > 6.1969-0.1 && score2 < 6.1970+0.1);

    // test 3: fuzzy search, complete match, single term
    //         query: "faugen"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/6 * 0.5 = 5/6*0.5
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity
    //                               = 6.52308630355 * 5/6 * 0.5
    //                               = 5.43590525296 * 0.5 = 2.7164
    float score3 = pingToGetTopScore(analyzer, indexSearcher, "faugen");
    cout << "Score: " << score3 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score3 > 2.7164-0.1 && score3 < 2.7164+0.1);

    // test 4: fuzzy search, prefix match, single term
    //         query: "fauge"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 * 0.5 = 4/5 * 0.5
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty
    //                               = 6.52308630355 * 0.95 * 4/5 * 0.5
    //                               = 4.9575455907 * 0.5 = 2.4774
    float score4 = pingToGetTopScore(analyzer, indexSearcher, "fauge");
    cout << "Score: " << score4 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score4 > 2.4774-0.1 && score4 < 2.4774+0.1);

    // test 5: fuzzy search, prefix match, double terms
    //         query: "fauge+medican"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   Term1:
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 *0.5= 4/5 * 0.5
    //                   term1_total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 6.52308630355 * 4/5 * 0.95 * 0.5
    //                                     = 4.9575455907 * 0.5
    //                   Term2:
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/7 = 6/7 * 0.5
    //                   term2_total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 6.52308630355 * 6/7 * 0.95 * 0.5
    //                                     = 5.31165599003 * 0.5
    //
    //                   total_score = term1_total_score + term2_total_score
    //                               = 10.26920158073
    float score5 = pingToGetTopScore(analyzer, indexSearcher, "fauge+medican");
    cout << "Score: " << score5 << endl;
    //After the change of float to half float in forward list, we will lose some precision, so we extend the interval
    ASSERT(score5 > 5.1318-0.1 && score5 < 5.1318+0.1);

}

void validateGeoIndexScoresExpression1(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    // Targeted Record:
    //    name - "Fargen Matthew MD"
    //    category - "Health & Medicine Physicians"
    //    record boost - 6.38022916069
    //    latitude - 38.230989
    //    longitude - -85.696596
    
    // Ranking Expression: idf_score * doc_boost

    // Query Range: left_bottom_latitude = 37.2,   right_top_latitude = 39.2
    //              left_bottom_longitude = -86.6, right_top_latitude = -84.6
    //              bonding_box_width = 2.0,       bonding_box_height = 2.0
    //              bonding_box_center_lat = 38.2, bonding_box_center_lng = -85.6

    // geo_score = max( DistanceRatio^2, MIN_DISTANCE_SCORE(0.05) )
    //           = 0.92826749224^2 = 0.86168053715
    // DistanceRatio = ( sqrt(minDist2UpperBound) - sqrt(resultMinDist2) ) /  sqrt(minDist2UpperBound)
    //               = ( sqrt(2) - sqrt(0.01029110534) ) / sqrt(2)
    //               = 0.92826749224
    // minDist2UpperBound  = max( searchRadius2 , MIN_SEARCH_RANGE_SQUARE(0.24*0.24) )
    //                     = 2
    // searchRadius2 = (bonding_box_width/2.0)^2 + ((bonding_box_height)/2.0)^2
    //               = 2
    // resultMinDist2  = (resultLat - bonding_box_center_lat)^2 + (resultLng - bonding_box_center_lng)^2
    //                 = (0.030989)^2 + (0.096596)^2
    //                 = 0.01029110534

    // test 1: exact search, complete match, single term
    //         query: "fargen"
    //         function: idf_score = sumOfFieldBoost
    //                             = (1 + 2/3)
    //                             = 5/3
    //                   total_score = idf_score * doc_boost * geo_score
    //                               = 5/3 * 6.38022916069 * 0.86168053715
    //                               = 9.16286548387
    float score1 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fargen",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score1 << endl;
    ASSERT(score1 > 9.1628-0.1 && score1 < 9.1629+0.1);

    // test 2: exact search, prefix match, single term
    //         query: "farge"
    //         function: idf_score = sumOfFieldBoost
    //                             = (1 + 2/3)
    //                             = 5/3
    //                   prefixMatchPenalty = 0.95
    //                   total_score = idf_score * doc_boost * prefixMatchPenalty * geo_score
    //                               = 5/3 * 6.38022916069 * 0.95 * 0.86168053715
    //                               = 8.70472220968
    float score2 = pingToGetTopScoreGeo(analyzer, indexSearcher, "farge",
                                     37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score2 << endl;
    ASSERT(score2 > 8.7047-0.1 && score2 < 8.7048+0.1);

    // test 3: fuzzy search, complete match, single term
    //         query: "faugen"
    //         function: idf_score = sumOfFieldBoost
    //                             = (1 + 2/3)
    //                             = 5/3
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/6 = 5/6 * 0.5
    //                   total_score = idf_score * doc_boost * NormalizedEdSimilarity * geo_score
    //                               = 5/3 * 6.38022916069 * 5/6 * 0.86168053715 * 0.5
    //                               = 7.63572123656 * 0.5
    float score3 = pingToGetTopScoreGeo(analyzer, indexSearcher, "faugen",
                                     37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score3 << endl;
    ASSERT(score3 > 3.8147-0.1 && score3 < 3.8147+0.1);

    // test 4: fuzzy search, prefix match, single term
    //         query: "fauge"
    //         function: idf_score = sumOfFieldBoost
    //                             = (1 + 2/3)
    //                             = 5/3
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 *0.5= 4/5 * 0.5
    //                   total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty * geo_score
    //                               = 5/3 * 6.38022916069 * 4/5 * 0.95 * 0.86168053715 * 0.5
    //                               = 6.96377776775 * 0.5
    float score4 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fauge",
                                     37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score4 << endl;
    ASSERT(score4 > 3.4790-0.1 && score4 < 3.4790+0.1);

    // test 5: fuzzy search, prefix match, double terms
    //         query: "fauge+medican"
    //         function: Term1:
    //                   idf_score = sumOfFieldBoost
    //                             = (1 + 2/3)
    //                             = 5/3
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 * 0.5 = 4/5 * 0.5
    //                   term1_total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 5/3 * 6.38022916069 * 4/5 * 0.95
    //                                     = 8.08162360354
    //                   Term2:
    //                   idf_score = sumOfFieldBoost
    //                             = (1 + 1/3)
    //                             = 4/3
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/7 = 6/7 * 0.5
    //                   term2_total_score = idf_score * doc_boost * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = 4/3 * 6.38022916069 * 6/7 * 0.95
    //                                     = 6.92710594589
    //
    //                   total_score = (term1_total_score + term2_total_score) * geo_score
    //                               = (8.08162360354 + 6.92710594589) * 0.86168053715
    //                               = 12.93273014009
    float score5 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fauge+medican",
                                     37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score5 << endl;
    ASSERT(score5 > 6.4610-0.1 && score5 < 6.4610+0.1);

}

void validateGeoIndexScoresExpression2(const Analyzer *analyzer, IndexSearcher *indexSearcher)
{
    // Targeted Record:
    //    name - "Fargen Matthew MD"
    //    category - "Health & Medicine Physicians"
    //    record boost - 6.38022916069
    //    latitude - 38.230989
    //    longitude - -85.696596
    
    // Ranking Expression: doc_boost + ( 1 / (doc_length+1) )

    // Query Range: left_bottom_latitude = 37.2,   right_top_latitude = 39.2
    //              left_bottom_longitude = -86.6, right_top_latitude = -84.6
    //              bonding_box_width = 2.0,       bonding_box_height = 2.0
    //              bonding_box_center_lat = 38.2, bonding_box_center_lng = -85.6

    // geo_score = max( DistanceRatio^2, MIN_DISTANCE_SCORE(0.05) )
    //           = 0.92826749224^2 = 0.86168053715
    // DistanceRatio = ( sqrt(minDist2UpperBound) - sqrt(resultMinDist2) ) /  sqrt(minDist2UpperBound)
    //               = ( sqrt(2) - sqrt(0.01029110534) ) / sqrt(2)
    //               = 0.92826749224
    // minDist2UpperBound  = max( searchRadius2 , MIN_SEARCH_RANGE_SQUARE(0.24*0.24) )
    //                     = 2
    // searchRadius2 = (bonding_box_width/2.0)^2 + ((bonding_box_height)/2.0)^2
    //               = 2
    // resultMinDist2  = (resultLat - bonding_box_center_lat)^2 + (resultLng - bonding_box_center_lng)^2
    //                 = (0.030989)^2 + (0.096596)^2
    //                 = 0.01029110534

    // test 1: exact search, complete match, single term
    //         query: "fargen"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * geo_score
    //                               = (6.38022916069 + ( 1/ (6+1) ) ) * 0.86168053715
    //                               = 5.62081650992
    float score1 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fargen",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score1 << endl;
    ASSERT(score1 > 5.6208-0.1 && score1 < 5.6209+0.1);

    // test 2: exact search, prefix match, single term
    //         query: "farge"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   prefixMatchPenalty = 0.95
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * prefixMatchPenalty * geo_score
    //                               = (6.38022916069 + ( 1/ (6+1) ) ) * 0.95 * 0.86168053715
    //                               = 5.33977568442
    float score2 = pingToGetTopScoreGeo(analyzer, indexSearcher, "farge",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score2 << endl;
    ASSERT(score2 > 5.3397-0.1 && score2 < 5.3398+0.1);

    // test 3: fuzzy search, complete match, single term
    //         query: "faugen"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/6 = 5/6
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * geo_score
    //                               = (6.38022916069 + ( 1/ (6+1) ) ) * 5/6 * 0.86168053715
    //                               = 4.68401375826
    float score3 = pingToGetTopScoreGeo(analyzer, indexSearcher, "faugen",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score3 << endl;
    ASSERT(score3 > 2.3407-0.1 && score3 < 2.3407+0.1);

    // test 4: fuzzy search, prefix match, single term
    //         query: "fauge"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 *0.5= 4/5 * 0.5
    //                   total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty * geo_score
    //                               = (6.38022916069 + ( 1/ (6+1) ) ) * 4/5 * 0.95 * 0.86168053715 * 0.5
    //                               = 4.27182054753 *0.5
    float score4 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fauge",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score4 << endl;
    ASSERT(score4 > 2.1347-0.1 && score4 < 2.1347+0.1);

    // test 5: fuzzy search, prefix match, double terms
    //         query: "fauge+medican"
    //         function: doc_boost = 6.38022916069
    //                   doc_length = 6
    //                   Term1:
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/5 *0.5 = 4/5 *0.5
    //                   term1_total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = (6.38022916069 + ( 1/ (6+1) ) ) * 4/5 * 0.95 *0.5
    //                                     = 4.9575455907 *0.5
    //                   Term2:
    //                   prefixMatchPenalty = 0.95
    //                   NormalizedEdSimilarity = (1 - ed/query_term_length ) * similarityBoost^ed
    //                                          = 1 - 1/7 = 6/7 * 0.5
    //                   term2_total_score = ( doc_boost + ( 1 / (doc_length+1) ) ) * NormalizedEdSimilarity * prefixMatchPenalty
    //                                     = (6.38022916069 + ( 1/ (6+1) ) ) * 6/7 * 0.95 * 0.5
    //                                     = 5.31165599003 *0.5
    //
    //                   total_score = (term1_total_score + term2_total_score) * geo_score
    //                               = (4.9575455907 + 5.31165599003) * 0.86168053715
    //                               = 8.84877113419
    float score5 = pingToGetTopScoreGeo(analyzer, indexSearcher, "fauge+medican",
                                        37.2, -86.6, 39.2, -84.6);
    cout << "Score: " << score5 << endl;
    ASSERT(score5 > 4.4219-0.1 && score5 < 4.4219+0.1);

}

void testScoreDefaultIndex(string index_dir, string data_file)
{
    // test scoring with one ranking expression: idf_score * doc_boost
    Indexer *indexer1 = buildIndex(data_file, index_dir, "idf_score*doc_boost");

    IndexSearcher *indexSearcher1 = IndexSearcher::create(indexer1);

    const Analyzer *analyzer1 = getAnalyzer();

    validateDefaultIndexScoresExpression1(analyzer1, indexSearcher1);

    delete indexSearcher1;
    delete indexer1;

    cout << "Default Index with ranking expression1 pass." << endl;

    // test scoring with another ranking expression: doc_boost + ( 1 / (doc_length+1) )
    Indexer *indexer2 = buildIndex(data_file, index_dir, "doc_boost+(1/(doc_length+1))");

    IndexSearcher *indexSearcher2 = IndexSearcher::create(indexer2);

    const Analyzer *analyzer2 = getAnalyzer();

    validateDefaultIndexScoresExpression2(analyzer2, indexSearcher2);

    delete indexSearcher2;
    delete indexer2;

    cout << "Default Index with ranking expression2 pass." << endl;
}

void testScoreGeoIndex(string index_dir, string data_file)
{
    // test scoring with one ranking expression: idf_score * doc_boost
    Indexer *indexer1 = buildGeoIndex(data_file, index_dir, "idf_score*doc_boost");

    IndexSearcher *indexSearcher1 = IndexSearcher::create(indexer1);

    const Analyzer *analyzer1 = getAnalyzer();

    validateGeoIndexScoresExpression1(analyzer1, indexSearcher1);

    delete indexSearcher1;
    delete indexer1;

    cout << "Geo Index with ranking expression1 pass." << endl;

    // test scoring with another ranking expression: doc_boost + ( 1 / (doc_length+1) )
    Indexer *indexer2 = buildGeoIndex(data_file, index_dir, "doc_boost+(1/(doc_length+1))");

    IndexSearcher *indexSearcher2 = IndexSearcher::create(indexer2);

    const Analyzer *analyzer2 = getAnalyzer();

    validateGeoIndexScoresExpression2(analyzer2, indexSearcher2);

    delete indexSearcher2;
    delete indexer2;

    cout << "Geo Index with ranking expression2 pass." << endl;
}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string data_file = index_dir + "/data";

    testScoreDefaultIndex(index_dir, data_file);
    testScoreGeoIndex(index_dir, data_file);

    return 0;
}
