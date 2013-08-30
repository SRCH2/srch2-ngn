
// $Id: DoubleQueryStress_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

// variables to measure the elapsed time
struct timespec tstart;
struct timespec tend;

struct timespec tstart_each;
struct timespec tend_each;

// variables to measure the elapsed time
struct timespec tstart_d;
struct timespec tend_d;

struct timespec tstart_d_each;
struct timespec tend_d_each;

bool parseLine(string &line, string &query, bool &returnValue1, bool &returnValue2)
{
    vector<string> record;
    csvline_populate(record, line, ',');

    if (record.size() < 3)
      return false;

    query = record[0];
    returnValue1 = atoi(record[1].c_str());
    returnValue2 = atoi(record[2].c_str());

    return true;
}

int main(int argc, char **argv)
{
    std::string index_dir = getenv("index_dir");

    // user defined query variables
    std::vector<std::string> file;
    std::vector<std::string> output_print;
    std::string line;
    file.clear();

    std::string query_file = getenv("query_file");
    std::ifstream infile(query_file.c_str(), std::ios_base::in);
    while (getline(infile, line, '\n')) {
        string query ="";
        bool returnValue1 = 0;
        bool returnValue2 = 0;
        bool pass = parseLine(line, query, returnValue1,returnValue2);
        if (!pass)
            abort();
        file.push_back (query);
    }

    clock_gettime(CLOCK_REALTIME, &tstart);

    {
        // create an index searcher
        //GlobalCache *cache = GlobalCache::create(100000,1000); // To test aCache
        Cache *cache = new Cache();// create an index writer
        unsigned mergeEveryNSeconds = 3;
        unsigned mergeEveryMWrites = 5;
        IndexMetaData *indexMetaData = new IndexMetaData(cache, mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
        Indexer *indexer = Indexer::load(indexMetaData);
        IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
        const Analyzer *analyzer = getAnalyzer();

        for (vector<string>::iterator vectIter = file.begin(); vectIter!= file.end(); vectIter ++) {
            //for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.begin()+200; vectIter++ )
                //std::cout << *vectIter <<  std::endl;
            clock_gettime(CLOCK_REALTIME, &tstart_each);

            unsigned resultCount = 10;
            pingNormalQuery(analyzer, indexSearcher,*vectIter,resultCount,0);

            clock_gettime(CLOCK_REALTIME, &tend_each);
            double ts2 = (tend_each.tv_sec - tstart_each.tv_sec) * 1000 + (tend_each.tv_nsec - tstart_each.tv_nsec)/1000000;

            std::ostringstream out;
            out << *vectIter << "," << ts2;
            output_print.push_back(out.str());
        }
        delete indexSearcher;
        delete analyzer;
    }

    clock_gettime(CLOCK_REALTIME, &tend);
    double ts2 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    cout << "Double Searcher Ping..." << endl;

    clock_gettime(CLOCK_REALTIME, &tstart_d);
    {
        // create an index searcher
        //GlobalCache *cache = GlobalCache::create(100000,1000); // To test aCache
        Cache *cache = new Cache();// create an index writer
        unsigned mergeEveryNSeconds = 3;
        unsigned mergeEveryMWrites = 5;
        IndexMetaData *indexMetaData = new IndexMetaData( cache, mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
        Indexer *indexer = Indexer::load(indexMetaData);
        IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
        const Analyzer *analyzer = getAnalyzer();
        for (vector<string>::iterator vectIter = file.begin(); vectIter!= file.end(); vectIter ++) {
            //for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.begin()+200; vectIter++ )
            clock_gettime(CLOCK_REALTIME, &tstart_d_each);

            unsigned resultCount = 10;
            std::cout << "[[" << *vectIter << "]]" << std::endl;
            doubleSearcherPing(analyzer, indexSearcher,*vectIter,resultCount,0);
    
            clock_gettime(CLOCK_REALTIME, &tend_d_each);
            double ts2 = (tend_d_each.tv_sec - tstart_d_each.tv_sec) * 1000 
                       + (tend_d_each.tv_nsec - tstart_d_each.tv_nsec) / 1000000;

            std::ostringstream out;
            out << *vectIter << "," << ts2;
            output_print.push_back(out.str());
        }
        delete indexSearcher;
        delete indexer;
        delete cache;
        delete analyzer;
    }

    cout << "Double Searcher Ping done." << endl;

    clock_gettime(CLOCK_REALTIME, &tend_d);
    double ts2_d = (tend_d.tv_sec - tstart_d.tv_sec) * 1000 + (tend_d.tv_nsec - tstart_d.tv_nsec) / 1000000;

    //delete index;
    cout << "Writing to file..." << endl;
    //ofstream outfile("/home/chrysler/DoubleQueryTest_output.txt", ios::out | ios::binary);
    //ofstream outfile(index_dir+"/DoubleQueryTest_output.txt", ios::out | ios::binary);
    ofstream outfile("DoubleQueryTest_output.txt", ios::out | ios::binary);
    for (std::vector<string>::iterator iter = output_print.begin(); iter != output_print.end(); ++iter)
        outfile << *iter << endl;

    cout << "Executed " << file.size() << " queries in " << ts2 << " milliseconds." << endl;
    cout << "Executed " << file.size() << " double_queries in " << ts2_d << " milliseconds." << endl;

    cout << "DoubleQueryStress_Test passed!" << endl;
    
    return 0;
}
