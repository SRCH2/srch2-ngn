
#include "index/ForwardIndex.h"
#include "record/SchemaInternal.h"
#include "index/Trie.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/Assert.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <cstring>
#include <assert.h>
#include <stdint.h>

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie Trie_Internal;

bool approximateFloatEqual(float x, float y)
{
    return abs(x-y) < 0.01;
}

int main(int argc, char *argv[])
{

    bool verbose = false;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0)
    {
        verbose = true;
    }

    ///Create Schema
    srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_authors", 1); // searchable text
    schema->setSearchableAttribute("article_title", 4); // searchable text

    // Create a record of 3 attributes
    Record *record = new Record(schema);
    record->setPrimaryKey(1000);
    record->setSearchableAttributeValue("article_authors", "Tom Jerry");
    record->setSearchableAttributeValue("article_title", "Tom Cat");
    record->setRecordBoost(20);

    /// Create an Analyzer
    AnalyzerInternal *analyzer = new StandardAnalyzer(NULL, NULL, NULL, NULL, "");
    analyzer->setTokenStream(analyzer->createOperatorFlow());
    map<string, TokenAttributeHits > tokenAttributeHitsMap;

    ///Tokenize the Record. TokenAttributeHitsMap
    analyzer->tokenizeRecord(record, tokenAttributeHitsMap);

    ///Initialise Index Structures
    Trie_Internal *trie = new Trie_Internal();
    ForwardIndex *forwardIndex = new ForwardIndex(schema);

    unsigned internalRecordId;
    forwardIndex->appendExternalRecordIdToIdMap(record->getPrimaryKey(),internalRecordId);
    ASSERT( forwardIndex->getInternalRecordIdFromExternalRecordId(record->getPrimaryKey(),internalRecordId) == true);

    ///Insert into Trie
    unsigned invertedIndexOffset = 0;
    for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
            mapIterator != tokenAttributeHitsMap.end();
            ++mapIterator)
    {
        /// add words to trie
        //std::cout << "word:" << mapIterator->first << std::endl;
        trie->addKeyword(mapIterator->first, invertedIndexOffset);
    }
    trie->commit();
    trie->finalCommit_finalizeHistogramInformation(NULL , NULL, 0);

    KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

    typedef shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr;
    trie->getTrieRootNode_ReadView(rootSharedPtr);
    TrieNode *root = rootSharedPtr->root;

    unsigned catId = trie->getTrieNodeFromUtf8String(root, "cat")->getId();
    unsigned jerryId = trie->getTrieNodeFromUtf8String(root, "jerry")->getId();
    unsigned tomId = trie->getTrieNodeFromUtf8String(root, "tom")->getId();

    keywordIdList.push_back( make_pair(catId, make_pair("cat", invertedIndexOffset) ) );
    keywordIdList.push_back( make_pair(jerryId, make_pair("jerry", invertedIndexOffset) ) );
    keywordIdList.push_back( make_pair(tomId, make_pair("tom", invertedIndexOffset) ) );

    /// Sort keywordList
    std::sort(keywordIdList.begin(), keywordIdList.end() );

    /// add record and keywordIdList to forwardIndex
    forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

    bool dummy = false;
    shared_ptr<vectorview<ForwardListPtr> > readView;
    forwardIndex->getForwardListDirectory_ReadView(readView);
    float keywordTfBoostProduct1 = forwardIndex->getForwardList(readView, 0, dummy)->getKeywordTfBoostProduct(0);
    float keywordTfBoostProduct2 = forwardIndex->getForwardList(readView, 0, dummy)->getKeywordTfBoostProduct(1);
    float keywordTfBoostProduct3 = forwardIndex->getForwardList(readView, 0, dummy)->getKeywordTfBoostProduct(2);

    ASSERT(approximateFloatEqual(keywordTfBoostProduct1, 1.8));
    ASSERT(approximateFloatEqual(keywordTfBoostProduct2, 1.2));
    ASSERT(approximateFloatEqual(keywordTfBoostProduct3, 2));

    (void)keywordTfBoostProduct1;
    (void)keywordTfBoostProduct2;
    (void)keywordTfBoostProduct3;

    delete forwardIndex;
    delete trie;
    delete analyzer;
    delete record;
    delete schema;

    cout << "FieldBoost Unit Test: Passed\n";

    return 0;
}
