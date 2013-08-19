
#ifndef __MAPSEARCHTESTHELPER_H__
#define __MAPSEARCHTESTHELPER_H__

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>

#include <query/QueryResultsInternal.h>
#include <operation/IndexSearcherInternal.h>

#include <boost/algorithm/string.hpp>

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
unsigned getNormalizedThresholdGeo(unsigned keywordLength);

bool parseLine(string &line, vector<string> &query, Rectangle &range)
;

bool ifAllFoundResultsAreCorrect(const vector<unsigned> &expectedResults, const vector<unsigned> &results)
;

bool ifAllExpectedResultsAreFound(const vector<unsigned> &expectedResults, const vector<unsigned> &results)
;

void readGeoRecordsFromFile(string filepath, Indexer *index, Schema *schema)
;

void printGeoResults(srch2is::QueryResults *queryResults, unsigned offset = 0)
;

float pingToGetTopScoreGeo(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng)
;

int pingToCheckIfHasResults(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng, int ed);


unsigned existsInTopKGeo(const Analyzer *analyzer, IndexSearcher *indexSearcher, string queryString, string primaryKey, int k, float lb_lat, float lb_lng, float rt_lat, float rt_lng)
;

#endif /* __MAPSEARCHTESTHELPER_H__ */
