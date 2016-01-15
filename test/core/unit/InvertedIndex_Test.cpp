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

#include "index/InvertedIndex.h"
#include "operation/IndexerInternal.h"
#include "operation/QueryEvaluatorInternal.h"
#include "analyzer/AnalyzerContainers.h"
#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <cstring>
#include "util/cowvector/compression/cowvector_S16.h"
#include <assert.h>

using namespace std;
using namespace srch2::instantsearch;


void addRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    SynonymContainer *syn = SynonymContainer::getInstance("", SYNONYM_DONOT_KEEP_ORIGIN);
    syn->init();
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, syn, "");
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000),
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
    syn->free();
}

bool test2()
{
    const string filename("testInvertedIndexSerialize");
    //PositionIndex *pos = new PositionIndex();

    //Schema *schema = Schema::create();
    ForwardIndex *forwardIndex = new ForwardIndex(NULL);
    //InvertedIndex *invertedIndex = new InvertedIndex(pos);
/*    InvertedIndex *invertedIndex = new InvertedIndex(forwardIndex);

    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);
    invertedIndex->incrementHitCount(0);

    invertedIndex->incrementHitCount(1);
    invertedIndex->incrementHitCount(1);

    invertedIndex->incrementHitCount(2);
    invertedIndex->incrementHitCount(2);
    invertedIndex->incrementHitCount(2);

    invertedIndex->incrementHitCount(3);
    invertedIndex->incrementHitCount(3);
    invertedIndex->incrementHitCount(3);


    const InvertedList *invertedList;


    invertedList = invertedIndex->getInvertedList(0);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 7);

    invertedList = invertedIndex->getInvertedList(1);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 2);

    invertedList = invertedIndex->getInvertedList(2);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 3);

    invertedList = invertedIndex->getInvertedList(3);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 3);

    //Testing Serialization
    InvertedIndex::save(*invertedIndex,filename);
    delete invertedIndex;

    InvertedIndex *invertedIndexDeserialized = new InvertedIndex(forwardIndex);
    InvertedIndex::load(*invertedIndexDeserialized, filename);

    invertedList = invertedIndexDeserialized->getInvertedList(0);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 7);

    invertedList = invertedIndexDeserialized->getInvertedList(1);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 2);

    invertedList = invertedIndexDeserialized->getInvertedList(2);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 3);

    invertedList = invertedIndexDeserialized->getInvertedList(3);
    //ASSERT(invertedList->offset == 0);
    //ASSERT(invertedList->size() == 3);


    delete invertedIndexDeserialized;
*/
    delete forwardIndex;

    return true;
}

bool test3()
{
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    
    Indexer *indexer = Indexer::load(indexMetaData);

    QueryEvaluatorRuntimeParametersContainer runTimeParameters;
    QueryEvaluatorInternal * queryEvaluatorInternal = new QueryEvaluatorInternal(dynamic_cast<IndexReaderWriter *>(indexer), &runTimeParameters);
    //(void)indexSearcherInternal;
    //indexSearcherInternal->getInvertedIndex()->print_test();
    delete queryEvaluatorInternal;
    //delete indexerInternal;
    //delete indexer;
    delete indexer;
    delete indexMetaData;
    return true;
}

int main(int argc, char *argv[])
{

    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0)
    {
        verbose = true;
    }
    verbose = true;

    addRecords();
    test2();
    test3();

    ///checking committing inverted lists is in IndexIntegrationTest

    cout << "InvertedIndex Unit Tests: Passed\n";

    return 0;
}
