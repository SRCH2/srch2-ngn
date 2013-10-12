
//$Id: FieldBoost_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

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
    AnalyzerInternal *analyzer = new StandardAnalyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");
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
    trie->finalCommit(NULL , 0);

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
    float boost1 = forwardIndex->getForwardList(0, dummy)->getKeywordRecordStaticScore(0);
    float boost2 = forwardIndex->getForwardList(0, dummy)->getKeywordRecordStaticScore(1);
    float boost3 = forwardIndex->getForwardList(0, dummy)->getKeywordRecordStaticScore(2);

    ASSERT(approximateFloatEqual(boost1, 1.8));
    ASSERT(approximateFloatEqual(boost2, 1.2));
    ASSERT(approximateFloatEqual(boost3, 2));

    (void)boost1;
    (void)boost2;
    (void)boost3;

    delete forwardIndex;
    delete trie;
    delete analyzer;
    delete record;
    delete schema;

    cout << "FieldBoost Unit Test: Passed\n";

    return 0;
}
