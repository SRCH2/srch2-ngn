//$Id: InvertedIndex_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $

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

#include "index/InvertedIndex.h"
#include "operation/IndexerInternal.h"
#include "operation/IndexSearcherInternal.h"
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

    Analyzer *analyzer = Analyzer::create(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "", "", "");
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    
    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);
    
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setRecordBoost(10);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setRecordBoost(20);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setRecordBoost(30);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setRecordBoost(40);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setSearchableAttributeValue(1, "Shinesyponzi on Wish you were here");
    record->setRecordBoost(50);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setRecordBoost(60);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setRecordBoost(80);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

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
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    
    Indexer *indexer = Indexer::load(indexMetaData);
    IndexSearcherInternal *indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));

    //(void)indexSearcherInternal;
    //indexSearcherInternal->getInvertedIndex()->print_test();
    delete indexSearcherInternal;
    //delete indexerInternal;
    //delete indexer;
    delete indexer;
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
