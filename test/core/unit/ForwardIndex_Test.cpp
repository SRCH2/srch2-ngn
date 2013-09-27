
//$Id: ForwardIndex_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include "index/ForwardIndex.h"
#include "record/SchemaInternal.h"
#include "index/Trie.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/Assert.h"

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

typedef Trie<char> Trie_Internal;

int main(int argc, char *argv[])
{

    bool verbose = false;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0)
    {
        verbose = true;
    }

    const string filename("testForwardIndexSerialize");

    ///Create Schema
	srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
	schema->setPrimaryKey("article_id"); // integer, not searchable
	//schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create a record of 3 attributes
	Record *record = new Record(schema);
	record->setPrimaryKey(1000);
	record->setSearchableAttributeValue("article_authors", "cancer canada canteen");
	record->setSearchableAttributeValue("article_title", "can cat and cans");
	record->setRecordBoost(20);

	/// Create an Analyzer
	AnalyzerInternal *analyzer = new AnalyzerInternal(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, " ");
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
	//trie->print_Trie();

	KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

	typedef shared_ptr<TrieRootNodeAndFreeList<char> > TrieRootNodeSharedPtr;
	TrieRootNodeSharedPtr rootSharedPtr;
	trie->getTrieRootNode_ReadView(rootSharedPtr);
	TrieNode<char> *root = rootSharedPtr->root;

	unsigned cancerId = trie->getTrieNode(root, "cancer")->getId();
	unsigned canadaId = trie->getTrieNode(root, "canada")->getId();
	unsigned canteenId = trie->getTrieNode(root, "canteen")->getId();
	unsigned canId = trie->getTrieNode(root, "can")->getId();
	unsigned catId = trie->getTrieNode(root, "cat")->getId();
	unsigned andId = trie->getTrieNode(root, "and")->getId();
	unsigned cansId = trie->getTrieNode(root, "cans")->getId();

	keywordIdList.push_back( make_pair(cancerId, make_pair("cancer", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(canadaId, make_pair("canada", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(canteenId, make_pair("canteen", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(canId, make_pair("can", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(catId, make_pair("cat", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(andId, make_pair("and", invertedIndexOffset) ) );
	keywordIdList.push_back( make_pair(cansId, make_pair("cans", invertedIndexOffset) ) );

	/// Sort keywordList
	std::sort(keywordIdList.begin(), keywordIdList.end() );

	/// add record and keywordIdList to forwardIndex
	forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

/*
    string prefix1;
    string prefix2;
    verbose = true;

    vector<unsigned> keywordIdList;
    vector<unsigned> positionIndexOffsetList;
    //vector<unsigned> testVector1, testVector2, testVector3;
    vector<unsigned>::const_iterator vectorIterator;

    keywordIdList.push_back(16);
    keywordIdList.push_back(23);
    keywordIdList.push_back(50);

    positionIndexOffsetList.push_back(1);
    positionIndexOffsetList.push_back(1);
    positionIndexOffsetList.push_back(1);

    srch2is::IndexType type = srch2is::DefaultIndex;

    srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(type));
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text

    // create a junk record of 3 attributes
    srch2is::Record *record = new srch2is::Record(schema);
    record->setPrimaryKey(1001);
    record->setRecordBoost(20);

    //PositionIndex *pos = new PositionIndex();
    //ForwardIndex *forwardIndex = new ForwardIndex(schema, pos);
    ForwardIndex *forwardIndex = new ForwardIndex(schema);
    forwardIndex->addRecord(record, 0, keywordIdList, positionIndexOffsetList);

    forwardIndex->addRecord(record, 1, keywordIdList, positionIndexOffsetList);

    keywordIdList.clear();
    keywordIdList.push_back(6);
    keywordIdList.push_back(25);
    forwardIndex->addRecord(record, 2, keywordIdList, positionIndexOffsetList);

    keywordIdList.push_back(36);
    forwardIndex->addRecord(record, 3, keywordIdList, positionIndexOffsetList);
*/

    float score = 0;
    score++; //to prevent unused variable error

    unsigned keywordId = 1;

    /* Origin test cases
     * assert(forwardIndex->haveWordInRange(0, 23, 23, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, 23, 24, -1 , keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, 10, 30, -1 , keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, 0, 0, -1 , keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, 51, 55, -1 , keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, 17, 22, -1 , keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, 45, 50, -1 , keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, 45, 55, -1 , keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, 10, 30, -1 , keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(1, 26, 33, -1 , keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(3, 35, 40, -1 , keywordId, score) == true);*/

    assert(forwardIndex->haveWordInRange(0, andId-1, andId, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, andId, andId, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, andId, andId+1, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, andId-1, andId+1, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, canId+1, cancerId-1, -1, keywordId, score) == true);
    assert(forwardIndex->haveWordInRange(0, cansId-1, catId+1, -1, keywordId, score) == true);

    assert(forwardIndex->haveWordInRange(0, andId-2, andId-1, -1, keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, andId+1, canId-1, -1, keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, canadaId+1, cancerId-1, -1, keywordId, score) == false);
    assert(forwardIndex->haveWordInRange(0, catId+1, catId+2, -1, keywordId, score) == false);

    /// Testing Serialization
    ForwardIndex::save(*forwardIndex, filename);
    delete forwardIndex;
    delete trie;
    delete record;
    //delete pos;
    //delete schema;

    //ForwardIndex *forwardIndexDeserialized = new ForwardIndex(schema, NULL);
    ForwardIndex *forwardIndexDeserialized = new ForwardIndex(schema);
    ForwardIndex::load(*forwardIndexDeserialized, filename);

    /* Origin test cases
    assert(forwardIndexDeserialized->haveWordInRange(0, 23, 23, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(0, 23, 24, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(0, 10, 30, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(0, 0, 0, -1 , keywordId, score) == false);
    assert(forwardIndexDeserialized->haveWordInRange(0, 51, 55, -1 , keywordId, score) == false);
    assert(forwardIndexDeserialized->haveWordInRange(0, 17, 22, -1 , keywordId, score) == false);
    assert(forwardIndexDeserialized->haveWordInRange(0, 45, 50, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(0, 45, 55, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(0, 10, 30, -1 , keywordId, score) == true);
    assert(forwardIndexDeserialized->haveWordInRange(1, 26, 33, -1 , keywordId, score) == false);
    assert(forwardIndexDeserialized->haveWordInRange(3, 35, 40, -1 , keywordId, score) == true);*/

    assert(forwardIndex->haveWordInRange(0, andId-1, andId, -1, keywordId, score) == true);
	assert(forwardIndex->haveWordInRange(0, andId, andId, -1, keywordId, score) == true);
	assert(forwardIndex->haveWordInRange(0, andId, andId+1, -1, keywordId, score) == true);
	assert(forwardIndex->haveWordInRange(0, andId-1, andId+1, -1, keywordId, score) == true);
	assert(forwardIndex->haveWordInRange(0, canId+1, cancerId-1, -1, keywordId, score) == true);
	assert(forwardIndex->haveWordInRange(0, cansId-1, catId+1, -1, keywordId, score) == true);

	assert(forwardIndex->haveWordInRange(0, andId-2, andId-1, -1, keywordId, score) == false);
	assert(forwardIndex->haveWordInRange(0, andId+1, canId-1, -1, keywordId, score) == false);
	assert(forwardIndex->haveWordInRange(0, canadaId+1, cancerId-1, -1, keywordId, score) == false);
	assert(forwardIndex->haveWordInRange(0, catId+1, catId+2, -1, keywordId, score) == false);

    ///checking committing forward lists is in IndexIntegrationTest
    (void)keywordId;

    delete forwardIndexDeserialized;

    cout << "ForwardIndex Unit Tests: Passed\n";

    return 0;
}
