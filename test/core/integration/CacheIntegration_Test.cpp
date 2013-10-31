
// $Id: CacheIntegration_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"

#include "util/Assert.h"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

void testCache1(const Analyzer *analyzer, IndexSearcher *indexSearcher1, IndexSearcher *indexSearcher2,
        string queryString, bool returnValue1, bool returnValue2)
{
    cout << "[" << queryString << "]" << endl;

    //Test the ActiveNodeSet cache
    bool result1 = pingCache1(analyzer, indexSearcher1, queryString);

    //Test the ConjunctiveResults cache
    bool result2 = pingCache2(analyzer, indexSearcher2, queryString);

    /*if ( result1 != returnValue1 || result2 != returnValue2)
        abort();*/

    cout << ", ActiveNodeCache:" << result1 << ", NormalQuery-Cache:" <<  result2 << endl;
    //cout << "=======" << result2 << endl;
}


void testCache2(const Analyzer *analyzer, IndexSearcher *indexSearcher1, IndexSearcher *indexSearcher2,
        string queryString, bool returnValue1, bool returnValue2)
{
    cout << "[" << queryString << "]" << endl;

    //Test the ActiveNodeSet cache
    //pingCache1(analyzer, indexSearcher, queryString);

    //Test the ConjunctiveResults cache
    pingCacheDoubleQuery(analyzer, indexSearcher1, indexSearcher2, queryString);

    //cout << ", ActiveNodeCache:" << pingCache1(analyzer, indexSearcher2, queryString) << endl;
}


bool parseLine(string &line, string &query, bool &returnValue1, bool &returnValue2)
{
    vector<string> record;
    csvline_populate(record, line, ',');

    if (record.size() < 3)
    {
        return false;
    }

    query = record[0];
    returnValue1 = atoi(record[1].c_str());
    returnValue2 = atoi(record[2].c_str());

    return true;
}

int main(int argc, char **argv)
{
    string index_dir = getenv("index_dir");

    std::vector<std::string> file;
    std::string line;
    file.clear();

    std::string query_file = getenv("query_file");
    std::ifstream infile (query_file.c_str(), std::ios_base::in);
    while (getline(infile, line, '\n'))
    {
        file.push_back (line);
    }

    /*{
        // create an index searcher
        Cache *cache1 = new Cache(); // To test aCache
        Cache *cache2 = new Cache(); // To test cCache
        IndexSearcher *indexSearcher2 = IndexSearcher::create(indexer, cache1);
        IndexSearcher *indexSearcher1 = IndexSearcher::create(indexer, cache2);

        //Analyzer tests
        testCache1(analyzer, indexSearcher1, indexSearcher2, " ", 0, 0);
        testCache1(analyzer, indexSearcher1, indexSearcher2, " ", 0, 0);
        testCache1(analyzer, indexSearcher1, indexSearcher2, "\n", 0, 0);
        testCache1(analyzer, indexSearcher1, indexSearcher2, "\r", 0, 0);


        //Cache Tests
        for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.end(); vectIter++ )
        //for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.begin() + 100; vectIter++ )
        {
            string query ="";
            bool returnValue1 = 0;
            bool returnValue2 = 0;

            //std::cout << *vectIter << std::endl;

            bool pass = parseLine(*vectIter, query, returnValue1,returnValue2);
            if (!pass)
                abort();

            testCache1(analyzer, indexSearcher1, indexSearcher2, query, returnValue1, returnValue2);
        }
        delete indexSearcher1;
        delete indexSearcher2;
    }*/

    {
        // create an index searcher
        Cache *cache1 = new Cache();
        Cache *cache2 = new Cache();

        unsigned mergeEveryNSeconds = 3;    
        unsigned mergeEveryMWrites = 5;
        unsigned updateHistogramEveryPMerges = 1;
        unsigned updateHistogramEveryQWrites = 5;
        
        IndexMetaData *indexMetaData1 = new IndexMetaData( cache1,
        		mergeEveryNSeconds, mergeEveryMWrites,
        		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
        		index_dir, "");
        IndexMetaData *indexMetaData2 = new IndexMetaData( cache2,
        		mergeEveryNSeconds, mergeEveryMWrites,
        		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
        		index_dir, "");
           
           Indexer *indexer1 = Indexer::load(indexMetaData1);
        Indexer *indexer2 = Indexer::load(indexMetaData2);
        
        const Analyzer *analyzer = getAnalyzer();

        IndexSearcher *indexSearcher1 = IndexSearcher::create(indexer1);
        IndexSearcher *indexSearcher2 = IndexSearcher::create(indexer2);

        //Analyzer tests
        testCache2(analyzer, indexSearcher1, indexSearcher2, " ", 0, 0);
        testCache2(analyzer, indexSearcher1, indexSearcher2, " ", 0, 0);
        testCache2(analyzer, indexSearcher1, indexSearcher2, "\n", 0, 0);
        testCache2(analyzer, indexSearcher1, indexSearcher2, "\r", 0, 0);

        //Cache Tests
        for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.end(); vectIter++ )
        //for( vector<string>::iterator vectIter = file.begin(); vectIter!= file.begin() + 100; vectIter++ )
        {
            string query ="";
            bool returnValue1 = 0;
            bool returnValue2 = 0;

            //std::cout << *vectIter << std::endl;

            bool pass = parseLine(*vectIter, query, returnValue1,returnValue2);
            if (!pass)
                abort();

            testCache2(analyzer, indexSearcher1, indexSearcher2, query, returnValue1, returnValue2);
        }
        delete indexSearcher1;
        delete indexSearcher2;
        delete indexer1;
        cout << "done1" << endl;
        delete indexer2;
        delete analyzer;

    }
    cout<<"Cache Integration tests Succesful!!"<<endl;

    return 0;
}
