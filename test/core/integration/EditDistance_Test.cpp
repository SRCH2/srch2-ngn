\
// $Id: EditDistance_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"
#include "IntegrationTestHelper.h"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

//TODO To improve

void buildLocalIndex(string INDEX_DIR)
{
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setPrimaryKey("article_id"); // integer, not searchable

    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    //schema->setAttribute("article_title", 7); // searchable text

    // create an analyzer
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

    // create a record of 3 attributes
    Record *record = new Record(schema);
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "padhraic smyth");
    record->setRecordBoost(20);

    Cache *cache = new Cache();// create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);

    // add a record
    index->addRecord(record, analyzer);

    // create a record of 3 attributes
    record->clear();
    record = new Record(schema);
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue("article_authors", "abXXXXX XXXabXX XXXXXab");
    record->setRecordBoost(20);

    index->addRecord(record, analyzer);

/*
    // create another record
    record->clear();
    record->setPrimaryKey(1002);

    record->setAttributeValue(1, "patrick university of california irvine");
    record->setAttributeValue(2, "Here comes the sun");

    indexWriter->addRecord(record);

    record->clear();
    record->setPrimaryKey(1002);
    record->setAttributeValue(1, "raindrops falling on my head butch cassidy and sundance kid");
    record->setAttributeValue(2, "Here comes the sun");

    // add a record
    indexWriter->addRecord(record);
*/
    // build the index
    index->commit();
    index->save();

    delete record;
    delete index;
    delete analyzer;
    delete schema;
}

// test with swap operation
void test1()
{
    string INDEX_DIR = getenv("index_dir");
    buildLocalIndex(INDEX_DIR);

    //GlobalCache *cache = GlobalCache::create(100000,1000); // To test aCache
    Cache *cache = new Cache();// create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer *indexer = Indexer::load(indexMetaData);
    indexer->getSchema()->setSupportSwapInEditDistance(true);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    const Analyzer *analyzer = getAnalyzer();

    //Edit distance 0
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "smth" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smith" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smythe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001) == true);

    //swap operation
    ASSERT ( pingEd(analyzer, queryEvaluator, "msyth" , 1 , 1001) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "symth" , 1 , 1001) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smtyh" , 1 , 1001) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smyht" , 1 , 1001) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "mxyth" , 1 , 1001) == false);

    ASSERT ( ping(analyzer, queryEvaluator, "smytx" , 1 , 1001) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smy" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyt" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyth" , 1 , 1001) == true);


    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smi" , 0 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smit" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001) == true);
    ping(analyzer, queryEvaluator, "padraic" , 1 , 1001);

    //Edit distance 0 0
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrai" , 1 , 1001) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "padraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smy" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyt" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyth" , 1 , 1001) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic" , 1 , 1001 ) == true);

    //Edit distance 1 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smith" , 1 , 1001) == true);

    //Edit distance 0 1
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001) == true);

    //Edit distance 0 2
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smithe" , 1 , 1001) == true);

    //Edit distance 2 2 swap operation
    ASSERT ( ping(analyzer, queryEvaluator, "pahdraci+smithe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "aphdraic+smithe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pdarhaic+smithe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pahdaric+smithe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrhiac+smithe" , 1 , 1001) == true);

    // swap operation around the repetition of the same letters
    ASSERT ( pingEd(analyzer, queryEvaluator, "baXXXXX" , 1 , 1002) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXbaXX" , 1 , 1002) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXXXba" , 1 , 1002) == true);

    (void)analyzer;
    delete queryEvaluator;
    delete indexer;
    delete cache;
    delete analyzer;
}

// test without swap operation
void test2()
{
    string INDEX_DIR = getenv("index_dir");
    buildLocalIndex(INDEX_DIR);

    //GlobalCache *cache = GlobalCache::create(100000,1000); // To test aCache
    Cache *cache = new Cache();// create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer *indexer = Indexer::load(indexMetaData);
    indexer->getSchema()->setSupportSwapInEditDistance(false);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    const Analyzer *analyzer = getAnalyzer();

    //Edit distance 0
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "smth" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smith" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smythe" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001) == true);

    //swap operation
    ASSERT ( pingEd(analyzer, queryEvaluator, "msyth" , 1 , 1001) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "symth" , 1 , 1001) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smtyh" , 1 , 1001) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smyht" , 1 , 1001) == false);

    ASSERT ( ping(analyzer, queryEvaluator, "smytx" , 1 , 1001) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smy" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyt" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyth" , 1 , 1001) == true);


    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smi" , 0 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smit" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001) == true);
    ping(analyzer, queryEvaluator, "padraic" , 1 , 1001);

    //Edit distance 0 0
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padr" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padra" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrai" , 1 , 1001) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "padraic+s" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+sm" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smy" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyt" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyth" , 1 , 1001) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic" , 1 , 1001 ) == true);

    //Edit distance 1 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smith" , 1 , 1001) == true);

    //Edit distance 0 1
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001) == true);

    //Edit distance 0 2
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smithe" , 1 , 1001) == true);

    // swap operation around the repetition of the same letters
    ASSERT ( pingEd(analyzer, queryEvaluator, "baXXXXX" , 1 , 1002) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXbaXX" , 1 , 1002) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXXXba" , 1 , 1002) == false);

    (void)analyzer;
    delete queryEvaluator;
    delete indexer;
    delete cache;
    delete analyzer;
}

int main(int argc, char **argv)
{
    //buildLocalIndex();
    //test1();

    //buildUCIIndex();
    //buildIndex();
    test1();
    test2();

    cout<<"Edit Distance tests Succesful!!"<<endl;

    return 0;
}
