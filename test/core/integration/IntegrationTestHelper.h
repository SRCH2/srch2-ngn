//$Id: IntegrationTestHelper.h 3480 2013-06-19 08:00:34Z jiaying $

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

#ifndef __INTEGRATIONTESTHELPER_H__
#define __INTEGRATIONTESTHELPER_H__

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"

#include <query/QueryResultsInternal.h>
#include <operation/IndexSearcherInternal.h>

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;


/**
 * This function computes a normalized edit-distance threshold of a
 * keyword based on its length.  This threshold can be used to support
 * instant search (search as you type) as a user types in a keyword
 * character by character.  The intuition is that we want to allow
 * more typos for longer keywords. The following is an example:
 * <table>
 * <TR> <TD> Prefix Term </TD>  <TD> Normalized Edit-Distance Threshold </TD> </TR>
 * <TR> <TD> ele </TD> <TD> 0 </TD> </TR>
 * <TR> <TD> elep </TD> <TD> 1 </TD> </TR>
 * <TR> <TD> elepha </TD> <TD> 2 </TD> </TR>
 * <TR> <TD> elephant </TD> <TD> 2 </TD> </TR>
 * </table>
 */
unsigned getNormalizedThreshold(unsigned keywordLength);


void buildIndex(string index_dir);
void buildFactualIndex(string index_dir, unsigned docsToIndex);

// There are four kinds of parse methods in this test helper to test four kinds of queries {exact,fuzzy} * {prefix,complete}
// 1. parse a query to exact and prefix keywords
void parseExactPrefixQuery(const Analyzer *analyzer, Query *query, string queryString, int attributeIdToFilter = -1);

// 2. parse a query to exact and complete keywords
void parseExactCompleteQuery(const Analyzer *analyzer, Query *query, string queryString, int attributeIdToFilter = -1);

// 3. parse a query to fuzzy and prefix keywords
void parseFuzzyPrefixQuery(const Analyzer *analyzer, Query *query, string queryString, int attributeIdToFilter = -1);

// 4. parse a query to fuzzy and complete keywords
void parseFuzzyCompleteQuery(const Analyzer *analyzer, Query *query, string queryString, int attributeIdToFilter = -1);

void parseFuzzyQueryWithEdSet(const Analyzer *analyzer, Query *query, const string &queryString, int ed, srch2::instantsearch::TermType termType);

void printResults(srch2is::QueryResults *queryResults, unsigned offset = 0);

/// Added for stemmer
void printResults(srch2is::QueryResults *queryResults, bool &isStemmed, unsigned offset = 0);

//Test the ConjunctiveResults cache
bool pingCacheDoubleQuery(const Analyzer *analyzer, IndexSearcher *indexSearcher1, IndexSearcher *indexSearcher2, string queryString);
//Stress Test
bool doubleSearcherPing(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , unsigned recordID , int attributeIdToFilter = -1);

//Stress Test
bool pingNormalQuery(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , int attributeIdToFilter = -1);

bool checkResults(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs);

bool checkOutput(QueryResults *queryResults, unsigned numberofHits, bool isStemmed);


bool checkResults(QueryResults *queryResults, unsigned numberOfHits ,unsigned recordID);

// For Debugging while constructing test cases;
bool checkResults_DUMMY(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs);
bool pingGetAllResultsQuery(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter, int attributeIdToSort = -1)
;
void getGetAllResultsQueryResults(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, bool descending, vector<string> &recordIds, int attributeIdToFilter, int attributeIdToSort = -1)
;

//Test the ActiveNodeSet cache
bool pingCache1(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString)
;
//Test the ConjunctiveResults cache
bool pingCache2(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString)
;
bool ping_DUMMY(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;

bool ping_DUMMY(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , unsigned recordID , int attributeIdToFilter = -1)
;


bool ping(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;

bool ping(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , unsigned recordID , int attributeIdToFilter = -1)
;

bool pingExactPrefix(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;

bool pingFuzzyPrefix(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;

bool pingExactComplete(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;

bool pingFuzzyComplete(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs, int attributeIdToFilter = -1)
;
// fuzzy query by default
float pingToGetTopScore(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString)
;

bool pingForScalabilityTest(const Analyzer *analyzer, IndexSearcher *indexSearcher, const string &queryString, unsigned ed, srch2::instantsearch::TermType termType)
;

void pingDummyStressTest(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, unsigned numberofHits = 10)
;

bool topK1ConsistentWithTopK2(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, const unsigned k1, const unsigned k2)
;

bool existsInTopK(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, string primaryKey, const unsigned k)
;
unsigned pingExactTest(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString)
;

void csvline_populate(vector<string> &record, const string& line, char delimiter)
;

#endif /* __INTEGRATIONTESTHELPER_H__ */
