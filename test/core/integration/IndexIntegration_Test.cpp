
//$Id: IndexIntegration_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include "record/AnalyzerInternal.h"
#include "record/SchemaInternal.h"
#include "index/Trie.h"
#include "index/ForwardIndex.h"
#include "index/InvertedIndex.h"
#include "util/Assert.h"

#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>

#include <iostream>
#include <functional>
#include <vector>
#include <cstring>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie<char> Trie_Internal;

void printTokenAttributeHitsMap(map<string,TokenAttributeHits> *tokenAttributeHitsMap)
{
	LOG_REGION(0,
		   cout<<"\n\nTokenAttributeMap:" << endl;
	);

	for( map<string,TokenAttributeHits>::const_iterator mapIterator = tokenAttributeHitsMap->begin();
			mapIterator != tokenAttributeHitsMap->end();
			mapIterator++)
	{
		vector<unsigned>::const_iterator iter = mapIterator->second.attributeList.begin();

		LOG_REGION(0,
			   cout<<"\n"<<mapIterator->first<<":" << endl;
		);
		for (;iter != mapIterator->second.attributeList.end();iter++)
		{
			LOG_REGION(0,
					cout<<*iter<<"\t";
			);
		}
	}
}

int main(int argc, char *argv[]) {

	bool verbose = false;
	if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
		verbose = true;
	}

	///Create Schema
	srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_title", 7); // searchable text

	// Create a record of 3 attributes
	Record *record = new Record(schema);
	record->setPrimaryKey(1000);
	record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_title", "Yesterday Once More");
	record->setRecordBoost(20);

	/// Create an Analyzer
	AnalyzerInternal *analyzer = new AnalyzerInternal(srch2::instantsearch::NO_STEMMER_NORMALIZER, " ");
	RankerExpression *rankerExpression = new RankerExpression("doc_length");
	map<string, TokenAttributeHits > tokenAttributeHitsMap;

	///Tokenize the Record. TokenAttributeHitsMap
	analyzer->tokenizeRecord(record, tokenAttributeHitsMap);
	printTokenAttributeHitsMap(&tokenAttributeHitsMap);

	///Initialise Index Structures
	Trie_Internal *trie = new Trie_Internal();
	ForwardIndex *forwardIndex = new ForwardIndex(schema);
	InvertedIndex *invertedIndex = new InvertedIndex(forwardIndex);

	unsigned internalRecordId;
	forwardIndex->appendExternalRecordId_WriteView(record->getPrimaryKey(),internalRecordId);
	ASSERT( forwardIndex->getInternalRecordId_WriteView(record->getPrimaryKey(),internalRecordId) == true);


	///Insert into Trie
	KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

	for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
			mapIterator != tokenAttributeHitsMap.end();
			++mapIterator)
	{
		/// add words to trie
		//std::cout << "word:" << mapIterator->first << std::endl;
		unsigned invertedIndexOffset = 0;
		unsigned keywordId = trie->addKeyword(mapIterator->first, invertedIndexOffset);
		keywordIdList.push_back( make_pair(keywordId, make_pair(mapIterator->first, invertedIndexOffset) ) );
		invertedIndex->incrementHitCount(invertedIndexOffset);
	}
	std::sort(keywordIdList.begin(), keywordIdList.end() );

	//Sort keywordList
	forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

	unsigned trieId = 0;
	(void)trieId;

	/// Create Another Record
	record->clear();
	keywordIdList.clear();

	record->setPrimaryKey(2000);
	record->setSearchableAttributeValue(0, "George Harris");
	record->setSearchableAttributeValue(1, "Here comes the sun");

	tokenAttributeHitsMap.clear();

	analyzer->tokenizeRecord(record, tokenAttributeHitsMap);
	printTokenAttributeHitsMap(&tokenAttributeHitsMap);

	forwardIndex->appendExternalRecordId_WriteView(record->getPrimaryKey(),internalRecordId);
	ASSERT( forwardIndex->getInternalRecordId_WriteView(record->getPrimaryKey(),internalRecordId) == true);


	for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
			mapIterator != tokenAttributeHitsMap.end();
			++mapIterator)
	{
		/// add words to trie
		std::cout << "Added word: " << mapIterator->first << std::endl;
		unsigned invertedIndexOffset = 0;
		unsigned keywordId = trie->addKeyword(mapIterator->first, invertedIndexOffset);
		keywordIdList.push_back( make_pair(keywordId, make_pair(mapIterator->first, invertedIndexOffset) ) );
		invertedIndex->incrementHitCount(invertedIndexOffset);
	}
	std::sort(keywordIdList.begin(), keywordIdList.end() );

	//Sort keywordList
	forwardIndex->addRecord(record, internalRecordId, keywordIdList, tokenAttributeHitsMap);

	typedef ts_shared_ptr<TrieRootNodeAndFreeList<char> > TrieRootNodeSharedPtr;
	TrieRootNodeSharedPtr rootSharedPtr;
	trie->getTrieRootNode_ReadView(rootSharedPtr);
	TrieNode<char> *root = rootSharedPtr->root;

	ASSERT(trie->getTrieNode( root,"and")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"jack")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"lennon")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"more")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"once")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"smith")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"tom")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"yesterday")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"comes")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"george")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"harris")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"here")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"sun")->getId() == ++trieId);
	ASSERT(trie->getTrieNode( root,"the")->getId() == ++trieId);

	/*    ///Asserts for InvertedIndex
    invertedIndex->print_test();

    ///Asserts for ForwardIndex
    forwardIndex->print_test();

    ///Asserts for PositionIndex
    positionIndex->print_test();*/

	vector<unsigned> keywordPositions_ASSERT1;
	vector<unsigned> keywordPositions_ASSERT2;
	vector<unsigned> keywordPositions_ASSERT3;

	keywordPositions_ASSERT1.push_back(1);
	keywordPositions_ASSERT1.push_back(54);
	keywordPositions_ASSERT1.push_back(65);


	/// Commiting Index Structures
	trie->commit();
	//this->trie->print_Trie();

	const vector<unsigned> *oldIdToNewIdMapVector = trie->getOldIdToNewIdMapVector();

	//this->forwardIndex->print_test();
	forwardIndex->merge();
	const unsigned totalNumberofDocuments = forwardIndex->getTotalNumberOfForwardLists_ReadView();
	invertedIndex->initialiseInvertedIndexCommit();
	for (unsigned forwardIndexIter = 0; forwardIndexIter < totalNumberofDocuments; ++forwardIndexIter)
	{
		ForwardList *forwardList = forwardIndex->getForwardList_ForCommit(forwardIndexIter);
		vector<NewKeywordIdKeywordOffsetTriple> newKeywordIdKeywordOffsetTriple;
		forwardIndex->commit(forwardList, oldIdToNewIdMapVector, newKeywordIdKeywordOffsetTriple);
		invertedIndex->commit(forwardList, rankerExpression, forwardIndexIter, totalNumberofDocuments, schema, newKeywordIdKeywordOffsetTriple);
	}

	forwardIndex->finalCommit();
	invertedIndex->finalCommit();

	// Testing Commited Index Structures

	/// Trie -> KeywordIds are ordered by their lexicographic order
	ASSERT (trie->getTrieNode( root,"and")->getId() < trie->getTrieNode( root,"jack")->getId());
	ASSERT (trie->getTrieNode( root,"smith")->getId() < trie->getTrieNode( root,"tom")->getId());
	ASSERT (trie->getTrieNode( root,"once")->getId() < trie->getTrieNode( root,"yesterday")->getId());
	ASSERT (trie->getTrieNode( root,"and")->getId() < trie->getTrieNode( root,"harris")->getId());
	ASSERT (trie->getTrieNode( root,"here")->getId() < trie->getTrieNode( root,"sun")->getId());

	///Testing Commited InvertedIndex
	invertedIndex->print_test();
	///Testing Commited ForwardIndex
	forwardIndex->print_test();

	///Asserts for InvertedIndex

	const InvertedListElement *invertedListElement;

	unsigned invertedListOffset = trie->getTrieNode( root,"jack")->getInvertedListOffset();
	//invertedList = invertedIndex->getInvertedList(invertedListOffset);

	//ASSERT(invertedList->offset == 1);
	ASSERT(invertedIndex->getInvertedListSize_ReadView(invertedListOffset) == 1);

	invertedListElement = invertedIndex->getInvertedListElementByDirectory(invertedListOffset,0);

	//ASSERT(invertedListElement->recordId == 1000);
	//ASSERT(invertedListElement->score == 0);
	cout<<"aaa"<<" "<<invertedListElement->recordId<<" "<<invertedListElement->positionIndexOffset<<endl;
	//ASSERT(invertedListElement->positionIndexOffset == 2);

	//invertedListElement = invertedIndex->getInvertedListElementByDirectory(invertedListOffset, invertedList->size());

	//ASSERT(invertedListElement->recordId == 2000);
	//ASSERT(invertedListElement->score == 0);
	//ASSERT(invertedListElement->positionIndexOffset == 2);
	//cout<<"aaa"<<invertedListElement->score<<" "<<invertedListElement->recordId<<" "<<invertedListElement->positionIndexOffset<<endl;
	///Asserts for ForwardIndex
	///Create the keywordIds vector for record2
	vector<unsigned> keywordIdsVector;
	//record->setAttributeValue(0, "George Harris");
	//record->setAttributeValue(1, "Here comes the sun");
	keywordIdsVector.push_back(trie->getTrieNode( root,"george")->getId());
	keywordIdsVector.push_back(trie->getTrieNode( root,"harris")->getId());
	keywordIdsVector.push_back(trie->getTrieNode( root,"here")->getId());
	keywordIdsVector.push_back(trie->getTrieNode( root,"comes")->getId());
	keywordIdsVector.push_back(trie->getTrieNode( root,"sun")->getId());
	keywordIdsVector.push_back(trie->getTrieNode( root,"the")->getId());
	sort(keywordIdsVector.begin(),keywordIdsVector.end());


	/*	vector<unsigned> keywordIdsVector_Assert;
	//unsigned offset = forwardIndex->getForwardList(1)->offset;
	unsigned offset = 1;
	for (unsigned counter = 0; counter < forwardIndex->getForwardList(1)->numberOfKeywords; counter++)
	{
		keywordIdsVector_Assert.push_back(forwardIndex->getForwardListElementByDirectory(offset , counter)->keywordId);
	}
	ASSERT(keywordIdsVector_Assert == keywordIdsVector);*/

	delete analyzer;
	delete record;
	delete schema;
	delete trie;
	delete invertedIndex;
	delete forwardIndex;

	cout << "\nIndex Integration Tests: Passed\n";

	return 0;
}
