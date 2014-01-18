
//$Id: ForwardIndex_Performance_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

/**
  This test case is create for evaluating the performance of the ForwardIndex
  in the senario of using it in Facebook.

  The data used for this test case is from StateMedia.
   */

#include "index/ForwardIndex.h"
#include "record/SchemaInternal.h"
#include "index/Trie.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/Assert.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <cstring>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#define MIN_PREFIX_LENGTH       3
#define RAND_ACCESS_LOOP_NUMBER 500000


using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie Trie_Internal;

typedef pair<unsigned, unsigned> MinMaxPair;

struct RecordWithKeywordInfo
{
    Record *record;
    vector<string> keywordStringVector;
};

// Read records from file
// Insert keywords into trie
void readData(vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector, string filepath,
              Schema *schema, AnalyzerInternal *analyzer, ForwardIndex *forwardIndex, Trie_Internal *trie)
{
    ifstream data(filepath.c_str());
    string line;
    int cellCounter;
    while(getline(data,line))
    {
        Record *record = new Record(schema);

        cellCounter = 0;
        stringstream  lineStream(line);
        string        cell;

        // Read data from file
        // the file should have two fields, seperated by '^'
        // the first field is the primary key, the second field is a searchable attribute
        while(getline(lineStream,cell,'^') && cellCounter < 2 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else
            {
                record->setSearchableAttributeValue(0, cell);
            }

            cellCounter++;
        }

	    // Tokenize the Record and store the tokenized keywords in TokenAttributeHitsMap
	    map<string, TokenAttributeHits > tokenAttributeHitsMap;
	    analyzer->tokenizeRecord(record, tokenAttributeHitsMap);
	

	    // Insert keywords into Trie
	    for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
	        mapIterator != tokenAttributeHitsMap.end();
	        ++mapIterator)
	    {
            unsigned invertedIndexOffset = 0;
	        trie->addKeyword(mapIterator->first, invertedIndexOffset);
	    }

        // store the record pointer into the vector for later use
        // we will store each record's keywords later
        RecordWithKeywordInfo recordWithKeywordInfo;
        recordWithKeywordInfo.record = record;
        recordWithKeywordInfoVector.push_back(recordWithKeywordInfo);
    }

    data.close();

    cout << recordWithKeywordInfoVector.size() << " records loaded." << endl;
}

// use the records read from the file and the committed trie to build ForwardIndex
void buildForwardIndex(vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector,
		AnalyzerInternal *analyzer, ForwardIndex *forwardIndex, Trie_Internal *trie, const TrieNode *root)
{
   for(unsigned i = 0; i < recordWithKeywordInfoVector.size(); i++)
   { 
	    KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

        map<string, TokenAttributeHits > tokenAttributeHitsMap;
        analyzer->tokenizeRecord(recordWithKeywordInfoVector[i].record, tokenAttributeHitsMap);

        // read each record for keywords
	    for(map<string, TokenAttributeHits>::iterator mapIterator = tokenAttributeHitsMap.begin();
	        mapIterator != tokenAttributeHitsMap.end();
	        ++mapIterator)
	    {
	        unsigned keywordId = trie->getTrieNodeFromUtf8String(root,  mapIterator->first )->getId();
	        keywordIdList.push_back( make_pair(keywordId, make_pair(mapIterator->first, 0) ) );

            recordWithKeywordInfoVector[i].keywordStringVector.push_back( mapIterator->first );
	    }

	    // Sort keywordList
	    sort(keywordIdList.begin(), keywordIdList.end() );

    	// Add record and keywordIdList to forwardIndex
	    forwardIndex->addRecord(recordWithKeywordInfoVector[i].record, i, keywordIdList, tokenAttributeHitsMap);
   }
   cout << "ForwardIndex is built." << endl;
}

// get all the <minId, maxId> pairs for all the valid prefixes of a keyword
void getMinIdMaxIdVector(vector< MinMaxPair > &minMaxVector, string &keyword, const Trie_Internal *trie, const TrieNode *root)
{
    const TrieNode *node = trie->getTrieNodeFromUtf8String(root, keyword);
    unsigned minId = node->getMinId();
    unsigned maxId = node->getMaxId();

    minMaxVector.push_back( make_pair(minId, maxId) );

    while(keyword.size() >= MIN_PREFIX_LENGTH)
    {
    	keyword = keyword.substr(0, keyword.length() - 1);
        node = trie->getTrieNodeFromUtf8String(root, keyword);
        minId = node->getMinId();
        maxId = node->getMaxId();

        bool dup = false;
        for(unsigned i = 0; i < minMaxVector.size(); i++)
        {
            if(minMaxVector[i].first == minId && minMaxVector[i].second == maxId)
                dup = true;
        }

        if(!dup)
            minMaxVector.push_back( make_pair(minId, maxId) );
    }

}

// test Forwardlist look up with sequential accesses
// go through records one by one
// for each record, search every keyword of it on it's forwardlist
void sequentialAccessTest(const vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector, const ForwardIndex *forwardIndex, Trie_Internal *trie, const TrieNode *root)
{
    timespec t1;
    timespec t2;
    double time_span = 0.0;
    unsigned counter = 0;

    for(unsigned i = 0; i < recordWithKeywordInfoVector.size(); i++)
    {
        float score = 0;

        for(unsigned j = 0; j < recordWithKeywordInfoVector[i].keywordStringVector.size(); j++)
        {
            unsigned keywordId;
            unsigned attributeBitmap;
            string keyword = recordWithKeywordInfoVector[i].keywordStringVector[j];

            vector< MinMaxPair > minMaxVector;
            getMinIdMaxIdVector(minMaxVector, keyword, trie, root);

            for(unsigned k = 0; k < minMaxVector.size(); k++)
            {
                clock_gettime(CLOCK_REALTIME, &t1);

                bool valid = false;
				const ForwardList* fl = forwardIndex->getForwardList(i, valid);
                bool result = false;
                if(valid == true){
                	forwardIndex->haveWordInRange(i, fl, minMaxVector[k].first, minMaxVector[k].second, -1, keywordId, attributeBitmap, score);
                }

                clock_gettime(CLOCK_REALTIME, &t2);

                time_span += (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

                if(!result)
                    cout << minMaxVector[k].first << " " << minMaxVector[k].second << " not found at record NO." << i <<endl;

                counter++;
            }
        }
    }
    
    cout << "Sequential Access Test: ForwardIndex is checked " << counter << " times in " << time_span << " milliseconds." << endl;
    cout << "Each check costs " << time_span / (double)counter * 1000.0 << " microseconds" << endl;
}

// randomly get a record
void getNextRandomRecord(unsigned &recordId, const vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector)
{
    recordId = (unsigned)( (double)(recordWithKeywordInfoVector.size() - 1) * (double)rand() / (double)RAND_MAX );
}

// randomly choose a record, then randomly get a keyword from it
void getNextRandomKeyword(string &keyword, const vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector)
{   
    unsigned record_offset = 0;

    do {
        record_offset = (unsigned)( (double)(recordWithKeywordInfoVector.size() - 1) * (double)rand() / (double)RAND_MAX );
    } while (recordWithKeywordInfoVector[record_offset].keywordStringVector.size() == 0);
    
    unsigned keyword_offset = (unsigned)( (double)(recordWithKeywordInfoVector[record_offset].keywordStringVector.size() - 1) * (double)rand() / (double)RAND_MAX );

    keyword = recordWithKeywordInfoVector[record_offset].keywordStringVector[keyword_offset];

}

// test Forwardlist look up with random accesses
// search random keywords on random records Forwardlists
void randomAccessTest(const vector<RecordWithKeywordInfo> &recordWithKeywordInfoVector,
                          const ForwardIndex *forwardIndex, const Trie_Internal *trie, const TrieNode *root)
{
    timespec t1;
    timespec t2;
    double time_span = 0.0;
    unsigned negCounter = 0;
    unsigned posCounter = 0;

    while((negCounter + posCounter) < RAND_ACCESS_LOOP_NUMBER)
    {
        unsigned recordId;
        string keyword;
        unsigned keywordId;
        unsigned attributeBitmap;
        float score = 0;
        
        getNextRandomKeyword(keyword, recordWithKeywordInfoVector);

        vector< MinMaxPair > minMaxVector;
        getMinIdMaxIdVector(minMaxVector, keyword, trie, root);

        for(unsigned j = 0; j < minMaxVector.size(); j++)
        {
            getNextRandomRecord(recordId, recordWithKeywordInfoVector);
                
            clock_gettime(CLOCK_REALTIME, &t1);

            bool valid = false;
			const ForwardList* fl = forwardIndex->getForwardList(recordId, valid);
            bool result = false;
            if(valid == true){
            	forwardIndex->haveWordInRange(recordId,fl, minMaxVector[j].first, minMaxVector[j].second, -1, keywordId, attributeBitmap, score);
            }
        
            clock_gettime(CLOCK_REALTIME, &t2);

            time_span += (double)((t2.tv_sec - t1.tv_sec) * 1000) + ((double)(t2.tv_nsec - t1.tv_nsec)) / 1000000.0;

            if(result)
                posCounter++;
            else
                negCounter++;
        }
    }

    cout << "Random Access Test: ForwardIndex is checked " << posCounter + negCounter << " times in " << time_span << " milliseconds." << endl;
    cout << "Succeed " << posCounter << " times. Failed " << negCounter << " times." << endl;
    cout << "Each check costs " << time_span / (double)(posCounter + negCounter) * 1000.0 << " microseconds" << endl;
}

int main(int argc, char *argv[])
{
    const string filename = getenv("data_path");

    srand( (unsigned)time(0) );

    /// Create a Schema
	srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
	schema->setPrimaryKey("primaryKey"); // integer, not searchable
	schema->setSearchableAttribute("description", 2); // searchable text

	/// Create an Analyzer
	AnalyzerInternal *analyzer = new StandardAnalyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

	/// Initialise Index Structures
	Trie_Internal *trie = new Trie_Internal();
	ForwardIndex *forwardIndex = new ForwardIndex(schema);

    vector<RecordWithKeywordInfo> recordWithKeywordInfoVector;

    /// Read data from file to make records, and insert all the records to the trie
    readData(recordWithKeywordInfoVector, filename, schema, analyzer, forwardIndex, trie);

    /// Commit the trie
	trie->commit();
    trie->finalCommit_finalizeHistogramInformation(NULL , 0);

	typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
	TrieRootNodeSharedPtr rootSharedPtr;
	trie->getTrieRootNode_ReadView(rootSharedPtr);
	TrieNode *root = rootSharedPtr->root;

    /// Add all the records to ForwardIndex
    buildForwardIndex(recordWithKeywordInfoVector, analyzer, forwardIndex, trie, root);

    /// Run the tests
    sequentialAccessTest(recordWithKeywordInfoVector, forwardIndex, trie, root);
    randomAccessTest(recordWithKeywordInfoVector, forwardIndex, trie, root);

    /// Clean up
    for(unsigned i = 0; i < recordWithKeywordInfoVector.size(); i++)
        delete recordWithKeywordInfoVector[i].record;

    delete forwardIndex;
    delete trie;
    delete analyzer;
    delete schema;


    return 0;
}
