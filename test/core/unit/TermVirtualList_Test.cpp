
// $Id: TermVirtualList_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include "operation/TermVirtualList.h"
#include "operation/IndexerInternal.h"
#include "operation/IndexerInternal.h"
#include "operation/QueryEvaluatorInternal.h"
#include "analyzer/AnalyzerInternal.h"
#include "index/Trie.h"
#include "index/ForwardIndex.h"
#include "index/InvertedIndex.h"

#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/Assert.h"
#include "util/Logger.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <string>

using namespace std;
using namespace srch2::instantsearch;
using srch2::util::Logger;

typedef Trie Trie_Internal;
typedef IndexReaderWriter IndexerInternal;
/**
 * Keyword: Record
 *   Pink:  2, 5, 6
 *   Pinksyponzi: 4
 *
 *   Floyd: 2, 6
 *   Floydsyponzi: 4
 *
 *   Shine: 2, 3
 *   Shinesyponzi: 4
 */
void addRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);
    
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setRecordBoost(10);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setRecordBoost(20);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setRecordBoost(30);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setRecordBoost(40);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setSearchableAttributeValue(1, "Shinesyponzi on Wish you were here");
    record->setRecordBoost(50);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setRecordBoost(60);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setRecordBoost(80);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, analyzer);

    ///TODO: Assert that This record must not be added
    /// 1) Repeat of primary key
    /// 2) Check when adding junk data liek &*^#^%%
    /*record->clear();
    record->setPrimaryKey(1001);
    record->setAttributeValue(0, "jimi pink");
    record->setAttributeValue(1, "comes junk 2345 $%^^#");
    record->setBoost(100);
    indexer->addRecord(record);
     */
    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
    delete indexMetaData;
}

//TODO:Not complete, check case where termvirtualList is empty and change resultSet to a vector
bool checkResults( TermVirtualList *termVirtualList, set<unsigned> *resultSet)
{
    HeapItemForIndexSearcher *heapItem = new HeapItemForIndexSearcher();
    while (termVirtualList->getNext(heapItem))
    {
        Logger::debug("HeapPop:->RecordId: %d\tScore:%.5f", heapItem->recordId, heapItem->termRecordRuntimeScore);
        
        if (resultSet->count(heapItem->recordId) == true)
        {
            resultSet->erase(heapItem->recordId);
        }
    }
    delete heapItem;
    
    if (resultSet->size() == 0)
    return true;
    else
    return false;
}

void Test_Complete_Exact(QueryEvaluatorInternal *queryEvaluatorInternal)
{
    /**
     * Keyword: Record
     *   Pink:  2, 5, 6
     *
     *   Floyd: 2, 6
     *
     *   Shine: 2, 3
     */
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(2);
    resultSet0.insert(5);
    resultSet0.insert(6);

    resultSet1.insert(2);
    resultSet1.insert(6);

    resultSet2.insert(2);
    resultSet2.insert(3);

    // create a query
    string keywords[3] = {
            "pink","floyd","shine"
    };
    Logger::debug("***COMPLETE EXACT***");

    TermType termType = TERM_TYPE_COMPLETE;
    Logger::debug("Query:%s",(keywords[0]).c_str());
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet;
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    //indexSearcherInternal->getInvertedIndex()->print_test();

    TermVirtualList *termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
    		prefixActiveNodeSet.get(), term0);

    prefixActiveNodeSet->printActiveNodes(queryEvaluatorInternal->getTrie());
    //termVirtualList->print_test();

    ASSERT(checkResults( termVirtualList, &resultSet0));

    delete term0;
    delete termVirtualList;
    //delete prefixActiveNodeSet;

    Logger::debug(keywords[1].c_str());
    term0 = ExactTerm::create(keywords[1], termType, 1, 1);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(),   term0);

    ASSERT(checkResults(termVirtualList, &resultSet1));

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[2].c_str());
    term0 = ExactTerm::create(keywords[2], termType, 1, 1);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet2));

    //ASSERT (heapItem->recordId == 1);
    delete term0;
    delete termVirtualList;
    //delete keywords;

}

void Test_Prefix_Exact(QueryEvaluatorInternal *queryEvaluatorInternal)
{
    /**
     * Keyword: Record
     *   Pink:  2, 5, 6
     *   Pinksyponzi: 4
     *
     *   Floyd: 2, 6
     *   Floydsyponzi: 4
     *
     *   Shine: 2, 3
     *   Shinesyponzi: 4
     */
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(2);
    resultSet0.insert(5);
    resultSet0.insert(6);
    resultSet0.insert(4);

    resultSet1.insert(2);
    resultSet1.insert(6);
    resultSet1.insert(4);

    resultSet2.insert(2);
    resultSet2.insert(3);
    resultSet2.insert(4);
    // create a query
    string keywords[3] = {
            "pin","floy","shi"
    };

    Logger::debug("***PREFIX EXACT***");
    TermType termType = TERM_TYPE_PREFIX;
    Logger::debug("Query: %s", keywords[0].c_str());
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet;
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    TermVirtualList *termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
    		prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet0) == true);

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[1].c_str());
    term0 = ExactTerm::create(keywords[1], termType, 1, 1);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet1) == true);

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[2].c_str());
    term0 = ExactTerm::create(keywords[2], termType, 1, 1);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet2) == true);

    //ASSERT (heapItem->recordId == 1);
    delete term0;
    delete termVirtualList;
    //delete[] keywords;

}

void Test_Complete_Fuzzy(QueryEvaluatorInternal *queryEvaluatorInternal)
{
    /**
     * Keyword: Record
     *   Pink:  2, 5, 6
     *
     *   Floyd: 2, 6
     *
     *   Shine: 2, 3
     */
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(2);
    resultSet0.insert(5);
    resultSet0.insert(6);
    //resultSet0.insert(4);

    resultSet1.insert(2);
    resultSet1.insert(6);
    //resultSet1.insert(4);

    resultSet2.insert(2);
    resultSet2.insert(3);
    //resultSet2.insert(4);
    // create a query

    string keywords[3] = {
            "pgnk","flayd","sheine"
    };

    Logger::debug("***COMPLETE FUZZY***");
    TermType termType = TERM_TYPE_COMPLETE;
    Logger::debug("Query:%s", keywords[0].c_str());
    Term *term0 = FuzzyTerm::create(keywords[0], termType, 1, 1, 2);
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet;
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    TermVirtualList *termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet0) == true);

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[1].c_str());
    term0 = FuzzyTerm::create(keywords[1], termType, 1, 1, 2);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(),   term0);

    ASSERT(checkResults(termVirtualList, &resultSet1) == true);

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[2].c_str());
    term0 = FuzzyTerm::create(keywords[2], termType, 1, 1, 2);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
            prefixActiveNodeSet.get(),   term0);

    ASSERT(checkResults(termVirtualList, &resultSet2) == true);

    delete term0;
    delete termVirtualList;
    //delete keywords;
}

void Test_Prefix_Fuzzy(QueryEvaluatorInternal *queryEvaluatorInternal)
{
    /**
     * Keyword: Record
     *   Pink:  2, 5, 6
     *   Pinksyponzi: 4
     *
     *   Floyd: 2, 6
     *   Floydsyponzi: 4
     *
     *   Shine: 2, 3
     *   Shinesyponzi: 4
     */
    set<unsigned> resultSet0, resultSet1, resultSet2;
    resultSet0.insert(2);
    resultSet0.insert(5);
    resultSet0.insert(6);
    resultSet0.insert(4);

    resultSet1.insert(2);
    resultSet1.insert(6);
    resultSet1.insert(4);

    resultSet2.insert(2);
    resultSet2.insert(3);
    resultSet2.insert(4);

    string keywords[3] = {
            "pionn","fllio","shiii"
    };

    Logger::debug("***PREFIX FUZZY***");
    Logger::debug("Query: %s",keywords[0].c_str());
    TermType termType = TERM_TYPE_PREFIX;
    Term *term0 = FuzzyTerm::create(keywords[0], termType, 1, 1, 2);
    boost::shared_ptr<PrefixActiveNodeSet> prefixActiveNodeSet;
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    TermVirtualList *termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
    		prefixActiveNodeSet.get(),   term0);

    ASSERT(checkResults(termVirtualList, &resultSet0) == true);

    delete term0;
    delete termVirtualList;

    Logger::debug(keywords[1].c_str());
    term0 = FuzzyTerm::create(keywords[1], termType, 1, 1, 2);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
    		prefixActiveNodeSet.get(),   term0);

    ASSERT(checkResults(termVirtualList, &resultSet1) == true);

    delete term0;
    delete termVirtualList;


    Logger::debug(keywords[2].c_str());
    term0 = FuzzyTerm::create(keywords[2], termType, 1, 1, 2);
    prefixActiveNodeSet = queryEvaluatorInternal->computeActiveNodeSet( term0);
    termVirtualList = new TermVirtualList(queryEvaluatorInternal->getInvertedIndex(),
    		queryEvaluatorInternal->getForwardIndex(),
    		prefixActiveNodeSet.get(), term0);

    ASSERT(checkResults(termVirtualList, &resultSet2) == true);

    delete term0;
    delete termVirtualList;
    //delete keywords;
}

void TermVirtualList_Tests()
{
    addRecords();
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
       Indexer *indexer = Indexer::load(indexMetaData);
    
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer, &runtimeParameters);
    QueryEvaluatorInternal * queryEvaluatorInternal = queryEvaluator->impl;


    Test_Complete_Exact(queryEvaluatorInternal);
    Test_Prefix_Exact(queryEvaluatorInternal);
    Test_Complete_Fuzzy(queryEvaluatorInternal);
    Test_Prefix_Fuzzy(queryEvaluatorInternal);

    delete indexSearcherInternal;
    delete indexer;
    delete indexMetaData;
}

int main(int argc, char *argv[])
{
    TermVirtualList_Tests();
    Logger::debug("TermVirtualList unit tests passed!!");
    return 0;
}
