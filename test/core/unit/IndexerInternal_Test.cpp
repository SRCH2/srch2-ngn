//$Id: IndexerInternal_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include "operation/IndexerInternal.h"
#include "operation/IndexData.h"
#include "index/Trie.h"
#include "instantsearch/Schema.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Record.h"
#include "util/Assert.h"
#include "index/InvertedIndex.h"
#include <iostream>
#include <functional>
#include <vector>
#include <cstring>

#include <time.h>
#include <stdio.h>

using namespace std;
using namespace srch2::instantsearch;

void testIndexData()
{
    /// Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    /// Create Analyzer
    Analyzer *analyzer = Analyzer::create(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    /// Create IndexData
    string INDEX_DIR = ".";
    IndexData *indexData = IndexData::create(INDEX_DIR,
                                            analyzer,
                                            schema,
                                            "",
                                            srch2::instantsearch::DISABLE_STEMMER_NORMALIZER);

    Record *record = new Record(schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setRecordBoost(10);
    indexData->_addRecord(record);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setRecordBoost(90);
    indexData->_addRecord(record);

    indexData->_commit();
    //index->print_Index();

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Jimaai Hendaarix");
    record->setSearchableAttributeValue(1, "Littaale waaing");
    record->setRecordBoost(90);
    indexData->_addRecord(record);

    //index->print_Index();

    /// test Trie
    Trie_Internal *trie = indexData->trie;

    typedef ts_shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr;
    trie->getTrieRootNode_ReadView(rootSharedPtr);
    TrieNode *root = rootSharedPtr->root;

    (void)(root);

    ASSERT( trie->getTrieNodeFromUtf8String( root, "and")->getId() < trie->getTrieNodeFromUtf8String( root, "come")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "come")->getId() < trie->getTrieNodeFromUtf8String( root, "hendrix")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "hendrix")->getId() < trie->getTrieNodeFromUtf8String( root, "jack")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "jack")->getId() < trie->getTrieNodeFromUtf8String( root, "jimi")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "jimi")->getId() < trie->getTrieNodeFromUtf8String( root, "lennon")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "lennon")->getId() < trie->getTrieNodeFromUtf8String( root, "little")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "little")->getId() < trie->getTrieNodeFromUtf8String( root, "more")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "more")->getId() < trie->getTrieNodeFromUtf8String( root, "once")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "once")->getId() < trie->getTrieNodeFromUtf8String( root, "smith")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "smith")->getId() < trie->getTrieNodeFromUtf8String( root, "tom")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "tom")->getId() < trie->getTrieNodeFromUtf8String( root, "wing")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "wing")->getId() < trie->getTrieNodeFromUtf8String( root, "yesterday")->getId() );

    // we assume that there is no background thread does merge,
    // or even if there is such a background thread, it didn't have a chance to do the merge
    ASSERT( trie->getTrieNodeFromUtf8String( root, "jimaai") == NULL );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "Hendaarix") == NULL );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "Littaale") == NULL );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "waaing") == NULL );

    ASSERT( trie->getTrieNodeFromUtf8String( root, "j")->getMinId() == trie->getTrieNodeFromUtf8String( root, "jack")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "j")->getMaxId() == trie->getTrieNodeFromUtf8String( root, "jimi")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "ja")->getMinId() == trie->getTrieNodeFromUtf8String( root, "jack")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "ja")->getMaxId() == trie->getTrieNodeFromUtf8String( root, "jack")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "win")->getMinId() == trie->getTrieNodeFromUtf8String( root, "wing")->getId() );
    ASSERT( trie->getTrieNodeFromUtf8String( root, "win")->getMaxId() == trie->getTrieNodeFromUtf8String( root, "wing")->getId() );

    /// test ForwardIndex
    ForwardIndex *forwardIndex = indexData->forwardIndex;

    float score = 0;
    unsigned keywordId = 1;
    // define the attributeBitmap only in debug mode
#if ASSERT_LEVEL > 0
    unsigned attributeBitmap = 0;
#endif
    ASSERT( forwardIndex->haveWordInRange(0, trie->getTrieNodeFromUtf8String( root, "jack")->getId(),
                                             trie->getTrieNodeFromUtf8String( root, "lennon")->getId(), -1, keywordId, attributeBitmap, score) == true );
    ASSERT( forwardIndex->haveWordInRange(0, trie->getTrieNodeFromUtf8String( root, "smith")->getId() + 1,
                                             trie->getTrieNodeFromUtf8String( root, "tom")->getId() - 1, -1, keywordId, attributeBitmap, score) == false );
    ASSERT( forwardIndex->haveWordInRange(1, trie->getTrieNodeFromUtf8String( root, "hendrix")->getId(),
                                             trie->getTrieNodeFromUtf8String( root, "jimi")->getId(), -1, keywordId, attributeBitmap, score) == true );
    ASSERT( forwardIndex->haveWordInRange(1, trie->getTrieNodeFromUtf8String( root, "wing")->getId() + 1,
                                             trie->getTrieNodeFromUtf8String( root, "wing")->getId() + 2, -1, keywordId, attributeBitmap, score) == false );

    /// test InvertedIndex
    InvertedIndex *invertedIndex = indexData->invertedIndex;

    (void)(forwardIndex);
    (void)(invertedIndex);
    (void)score;
    (void)keywordId;

    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "and")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "come")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "hendrix")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "jack")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "jimi")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "lennon")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "little")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "more")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "once")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "smith")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "tom")->getInvertedListOffset() ) == 1);
    ASSERT(invertedIndex->getInvertedListSize_ReadView( trie->getTrieNodeFromUtf8String( root, "wing")->getInvertedListOffset() ) == 1);


    delete schema;
    delete record;
    delete analyzer;
    delete indexData;
}

void test1()
{
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setPrimaryKey("article_id"); // integer, not searchable

    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    // create an analyzer
    Analyzer *analyzer = Analyzer::create(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");
    
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    string INDEX_DIR = "test";
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    
    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);
    Record *record = new Record(schema);
    char* authorsCharStar = new char[30];
    char* titleCharStar = new char[30];

    //generate random characers
    srand ( time(NULL) );
    // create a record of 3 attributes
    for (unsigned i = 0; i < 1000; i++)
    {
        record->setPrimaryKey(i + 1000);

        sprintf(authorsCharStar,"John %cLen%cnon",(rand() % 50)+65,(rand() % 10)+65);
        string authors = string(authorsCharStar);
        record->setSearchableAttributeValue("article_authors", authors);

        sprintf(titleCharStar,"Yesterday %cOnc%ce %cMore",
                (rand()%59)+65, (rand()%59)+65, (rand()%10)+65);
        string title = string(titleCharStar);
        record->setSearchableAttributeValue("article_title", title);

        record->setRecordBoost(rand() % 100);
        index->addRecord(record, 0);

        // for creating another record
        record->clear();
    }

    // build the index
    index->commit();

    //indexer->printNumberOfBytes();

    delete[] authorsCharStar;
    delete[] titleCharStar;
    delete record;
    delete index;
    delete analyzer;
    delete schema;
}
void addRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text
    
    Record *record = new Record(schema);
    Analyzer *analyzer = Analyzer::create(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( NULL, mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);
    
    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setRecordBoost(10);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    index->commit();
    //index->commit();
    //index->print_Index();

    std::cout << "print 1 $$$$$$$$$$$$$$" << std::endl;

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Jimaai Hendaarix");
    record->setSearchableAttributeValue(1, "Littaale waaing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    //index->print_Index();

    std::cout << "print 2 $$$$$$$$$$$$$$" << std::endl;

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

void test3()
{
    addRecords();
    
    /// Test the Trie

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    string INDEX_DIR = ".";
    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    
    Indexer *indexer = Indexer::load(indexMetaData);

    //index->print_Index();

    Record *record = new Record(indexer->getSchema());
    record->setPrimaryKey(1999);
    record->setSearchableAttributeValue(0, "steve jobs");
    record->setSearchableAttributeValue(1, "stanford speech");
    record->setRecordBoost(90);
    indexer->addRecord(record, 0);
    indexer->merge_ForTesting();

/*    // create an index searcher
    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
    Analyzer *analyzer = indexer->getAnalyzer();

    indexer->print_Index();

    ASSERT ( ping(analyzer, indexSearcher, "tom" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , 1008) == true);
    ASSERT ( ping(analyzer, indexSearcher, "smith" , 1 , 1001) == true);
    ASSERT ( ping(analyzer, indexSearcher, "jobs" , 1 , 1999) == true);

    (void)analyzer;
    delete indexSearcher;*/

    delete indexer;
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }

    //test1();
    //test3();

    testIndexData();
    cout << "IndexerInternal Unit Tests: Passed\n";

    return 0;
}
