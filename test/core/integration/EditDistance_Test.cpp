/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    CacheManager *cache = new CacheManager();// create an index writer
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
    CacheManager *cache = new CacheManager();// create an index writer
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

    vector<unsigned> filterAttributes;
    //Edit distance 0
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "smth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smythe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //swap operation
    ASSERT ( pingEd(analyzer, queryEvaluator, "msyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "symth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smtyh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smyht" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "mxyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == false);

    ASSERT ( ping(analyzer, queryEvaluator, "smytx" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smy" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyt" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);


    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smi" , 0 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smit" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ping(analyzer, queryEvaluator, "padraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND);

    //Edit distance 0 0
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "padraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smy" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyt" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 0 1
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 0 2
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 2 2 swap operation
    ASSERT ( ping(analyzer, queryEvaluator, "pahdraci+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "aphdraic+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pdarhaic+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pahdaric+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrhiac+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    // swap operation around the repetition of the same letters
    ASSERT ( pingEd(analyzer, queryEvaluator, "baXXXXX" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXbaXX" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXXXba" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == true);

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
    CacheManager *cache = new CacheManager();// create an index writer
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

    vector<unsigned> filterAttributes;
    //Edit distance 0
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "smth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smythe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //swap operation
    ASSERT ( pingEd(analyzer, queryEvaluator, "msyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "symth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smtyh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "smyht" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == false);

    ASSERT ( ping(analyzer, queryEvaluator, "smytx" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smy" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyt" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);


    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padh" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smi" , 0 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smit" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ping(analyzer, queryEvaluator, "padraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND);

    //Edit distance 0 0
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "pad" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padr" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padra" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padrai" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    ASSERT ( ping(analyzer, queryEvaluator, "padraic+s" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+sm" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smy" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyt" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smyth" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 1 1
    ASSERT ( ping(analyzer, queryEvaluator, "padraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 0 1
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smith" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    //Edit distance 0 2
    ASSERT ( ping(analyzer, queryEvaluator, "padhraic+smithe" , 1 , 1001, filterAttributes, ATTRIBUTES_OP_AND) == true);

    // swap operation around the repetition of the same letters
    ASSERT ( pingEd(analyzer, queryEvaluator, "baXXXXX" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXbaXX" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == false);
    ASSERT ( pingEd(analyzer, queryEvaluator, "XXXXXba" , 1 , 1002, filterAttributes, ATTRIBUTES_OP_AND) == false);

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
