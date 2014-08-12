//$Id: IntegrationTestHelper.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#ifndef __INTEGRATIONTESTHELPER_H__
#define __INTEGRATIONTESTHELPER_H__

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/Query.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <../wrapper/SortFilterEvaluator.h>
#include "util/Assert.h"
#include "util/Logger.h"

#include <query/QueryResultsInternal.h>
#include "operation/QueryEvaluatorInternal.h"
#include "../unit/UnitTestHelper.h"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;


/**
 * This function computes a normalized edit-distance threshold of a
 * keyword based on its length.  This threshold can be used to support
 * instant search (search as you type) as a user types in a keyword
 * character by character.  The intuition is that we want to allow
 * more typos for longer keywords. The following is an example:
 * <table>
 * <TR> <TD> Prefix Term </TD>  <TD> Normalized Edit-Distance Threshold </TD> </TR>
 * <TR> <TD> ele </TD> <TD> 0 </TD> </TR>
 * <TR> <TD> elep </TD> <TD> 1 </TD> </TR>
 * <TR> <TD> elepha </TD> <TD> 2 </TD> </TR>
 * <TR> <TD> elephant </TD> <TD> 2 </TD> </TR>
 * </table>
 */
unsigned getNormalizedThreshold(unsigned keywordLength)
{
    // Keyword length:             [1,3]        [4,5]        >= 6
    // Edit-distance threshold:      0            1           2
    if (keywordLength < 4)
        return 0;
    else if (keywordLength < 6)
        return 1;
    return 2;
}


void buildIndex(string indexDir)
{
    string filepath = indexDir+"/dblp40000records.csv";
    std::ifstream data(filepath.c_str());

    vector<string> fields;
    fields.push_back("paperid");
    fields.push_back("title");
    fields.push_back("authors");
    fields.push_back("booktitle");
    fields.push_back("year");

    /// create a schema
    /// id typeid   last_name   first_name  type    title   department  description email   location    work_phone  url
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setPrimaryKey(fields[0]); // integer, not searchable

    /// TODO: assign better field boosts
    for(unsigned i = 1; i < fields.size() - 1 ; ++i)/// We are not indexing year
    {
        schema->setSearchableAttribute(fields[i]); /// searchable text
    }

    // create an analyzer
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, " ");

    // create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		indexDir);
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);
    
    Record *record = new Record(schema);

    std::string line;
    int cellCounter;
    std::getline(data,line);
    while(std::getline(data,line))
    {
        cellCounter = 0;
        std::stringstream  lineStream(line);
        std::string        cell;
        record->setInMemoryData(line.c_str(), line.length());

        while(std::getline(lineStream,cell,','))
        {
            cell.assign(cell,1,cell.length()-2);
            if (cellCounter == 0)
            {
                unsigned primaryKey = atoi(cell.c_str());
                //cout<<"(pkey->)"<<primaryKey<<"|";
                record->setPrimaryKey(primaryKey);
            }
            else
            {
                //cout<<"|"<<cell;
                //you have the cell
                if(cell.compare("") == 0)
                {
                    record->setSearchableAttributeValue(cellCounter,"");
                }
                else
                {
                    record->setSearchableAttributeValue(cellCounter,cell);
                }
            }
            cellCounter++;
        }
        //cout<<endl;
        // add the record
        indexer->addRecord(record, analyzer);
        record->clear();
    }
    // build the index
    indexer->commit();
    indexer->save();

    // after commit(), no more records can be added

    delete schema;
    delete analyzer;
}

void buildFactualIndex(string indexDir, unsigned docsToIndex)
{
    string filepath = indexDir+"whole-us-data";
    std::ifstream data(filepath.c_str());

    std::cout << filepath << std::endl;

    vector<string> fields;
    fields.push_back("placeid");
    fields.push_back("title");
    fields.push_back("categories");
    //fields.push_back("year");

    /// create a schema
    /// id typeid   last_name   first_name  type    title   department  description email   location    work_phone  url
    Schema *schema = Schema::create(srch2is::DefaultIndex);

    schema->setPrimaryKey(fields[0]); // integer, not searchable

    /// TODO: assign better field boosts
    for(unsigned i = 1; i < fields.size() ; ++i)/// We are not indexing year
    {
        schema->setSearchableAttribute(fields[i]); /// searchable text
    }

    // create an analyzer
    Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, " ");

    // create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new CacheManager(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		indexDir);
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);
    
    Record *record = new Record(schema);

    std::string line;
    int cellCounter;
    std::getline(data,line);
    cout << line << endl;

    unsigned docsCounter = 0;
    while(std::getline(data,line))
    {
        cellCounter = 0;
        std::stringstream  lineStream(line);
        std::string        cell;
        record->setInMemoryData(line.c_str(), line.length());

        while(std::getline(lineStream,cell,'^'))
        {
            if (cell.length() > 1)
            {
                cell.assign(cell,0,cell.length()-2);
                //cout << cell << endl;
                if (cellCounter == 0)
                {
                    //unsigned primaryKey = atoi(cell.c_str());
                    unsigned primaryKey = docsCounter;
                    //cout<<"(pkey->)"<<primaryKey;
                    record->setPrimaryKey(primaryKey);
                }
                else if (cellCounter == 3)
                {
                    //cout<<"|j|"<<cell;
                    //you have the cell
                    if(cell.compare("") == 0)
                    {
                        record->setSearchableAttributeValue(1,"");
                    }
                    else
                    {
                        record->setSearchableAttributeValue(1,cell);
                    }
                }
                else if (cellCounter == 7)
                {
                    //cout<<"|i|"<<cell;
                    // you have the cell
                    if(cell.compare("") == 0)
                    {
                        record->setSearchableAttributeValue(2,"");
                    }
                    else
                    {
                        record->setSearchableAttributeValue(2,cell);
                    }
                }
            }
            cellCounter++;
        }
        bool added = indexer->addRecord(record, analyzer);
        (void)added;
        //if (added == -1)
        //cout<< "Outside:"<< docsCounter << " " << endl;

        docsCounter++;
        if (docsCounter > docsToIndex)
        {
            break;
        }

        // add the record

        record->clear();
    }
    cout << "Read:" << docsCounter << endl;
    // build the index
    indexer->commit();
    indexer->save();

    // after commit(), no more records can be added

    delete schema;
    delete analyzer;
}

// There are four kinds of parse methods in this test helper to test four kinds of queries {exact,fuzzy} * {prefix,complete}
// 1. parse a query to exact and prefix keywords
void parseExactPrefixQuery(const Analyzer *analyzer, Query *query, string queryString,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_PREFIX;
        Term *term = ExactTerm::create(queryKeywords[i].term, termType, 1, 0.5);
        term->addAttributesToFilter(attributeIdToFilter, andOperation);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

// 2. parse a query to exact and complete keywords
void parseExactCompleteQuery(const Analyzer *analyzer, Query *query, string queryString,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_COMPLETE;
        Term *term = ExactTerm::create(queryKeywords[i].term, termType, 1, 0.5);
        term->addAttributesToFilter(attributeIdToFilter, andOperation);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

// 3. parse a query to fuzzy and prefix keywords
void parseFuzzyPrefixQuery(const Analyzer *analyzer, Query *query, string queryString,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_PREFIX;
        Term *term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, getNormalizedThreshold(queryKeywords[i].term.size()));
        term->addAttributesToFilter(attributeIdToFilter, andOperation);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

// 4. parse a query to fuzzy and complete keywords
void parseFuzzyCompleteQuery(const Analyzer *analyzer, Query *query, string queryString,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_COMPLETE;
        Term *term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, getNormalizedThreshold(queryKeywords[i].term.size()));
        term->addAttributesToFilter(attributeIdToFilter, andOperation);
        //query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

void parseFuzzyQueryWithEdSet(const Analyzer *analyzer, Query *query, const string &queryString, int ed)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    srch2is::TermType termType = TERM_TYPE_COMPLETE;
    for (unsigned i = 0; i < queryKeywords.size(); ++i){
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        
        Term *term;
        if(i == (queryKeywords.size()-1)){
            termType = TERM_TYPE_PREFIX;
        }

        if(ed==0){
            term = ExactTerm::create(queryKeywords[i].term, termType, 1, 0.5);
        }
        else{
            term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, ed);
        }
        term->addAttributesToFilter(vector<unsigned>(), true);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

void printResults(srch2is::QueryResults *queryResults, unsigned offset = 0)
{
    cout << "Number of hits:" << queryResults->getNumberOfResults() << endl;
    for (unsigned resultIter = offset;
            resultIter < queryResults->getNumberOfResults() ; resultIter++)
    {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        // For each result, get the matching keywords and their edit distances
        queryResults->getMatchingKeywords(resultIter, matchingKeywords);
        queryResults->getEditDistances(resultIter, editDistances);

        // Output the result information
        cout << "\nResult-(" << resultIter << ") RecordId:"
             << queryResults->getRecordId(resultIter)
             << "\tScore:" << queryResults->getResultScoreString(resultIter);

        cout << "\nMatching Keywords:" << endl;
        unsigned editDistancesIter = 0;
        for(vector<string>::iterator iter = matchingKeywords.begin();
                iter != matchingKeywords.end(); iter ++, editDistancesIter ++)
        {
            cout << "\t"
                 << *iter << " "
                 << editDistances.at(editDistancesIter);
        }
        cout<< "["<<  queryResults->getInMemoryRecordString(resultIter) <<"]" << endl;
    }
}

/// Added for stemmer
void printResults(srch2is::QueryResults *queryResults, bool &isStemmed, unsigned offset = 0)
{
    cout << "Number of hits:" << queryResults->getNumberOfResults() << endl;
    for (unsigned resultIter = offset;
            resultIter < queryResults->getNumberOfResults() ; resultIter++)
    {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        // For each result, get the matching keywords and their edit distances
        queryResults->getMatchingKeywords(resultIter, matchingKeywords);
        queryResults->getEditDistances(resultIter, editDistances);

        // Output the result information
        cout << "\nResult-(" << resultIter << ") RecordId:"
             << queryResults->getRecordId(resultIter)
             << "\tScore:" << queryResults->getResultScoreString(resultIter);

        cout << "\nMatching Keywords:" << endl;
        unsigned editDistancesIter = 0;
        for(vector<string>::iterator iter = matchingKeywords.begin();
                iter != matchingKeywords.end(); iter ++, editDistancesIter ++)
        {
            cout << "\t"
                 << *iter << " "
                 << editDistances.at(editDistancesIter);
        }
        cout<< "["<<  queryResults->getInMemoryRecordString(resultIter) <<"]" << endl;
    //    cout<<endl;
    //    cout<<boolalpha;
    //    cout << "Is the record stemmed?  "<< isStemmed << endl;
    }
}

//Stress Test
bool doubleSearcherPing(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , unsigned recordID ,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    /*
    vector<unsigned> rIDList;
    rIDList.push_back(recordID);
    */
    //bool cacheHit_exact , fuzzy_reached, cacheHit_fuzzy = false;

    unsigned offset = 0;
    unsigned resultsToRetrieve = numberofHits;

    Query *exactQuery = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, exactQuery, queryString, attributeIdToFilter, andOperation);
    Query *fuzzyQuery = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, fuzzyQuery, queryString, attributeIdToFilter, andOperation);
    QueryResults *exactQueryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, exactQuery);
    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(exactQuery , fuzzyQuery, 0, offset + resultsToRetrieve, true, srch2::instantsearch::SearchTypeTopKQuery);
    unsigned idsExactFound = queryEvaluator->search(logicalPlan , exactQueryResults);

    exactQueryResults->printStats();

    printResults(exactQueryResults);

    // compute elapsed time in ms
/*    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;*/

    //MYSQL mysql;
    //getMySQLConnection(cReader, mysql);

    //printResults(conn, request_info, searchType, cReader, queryResults,
    //        offset, idsFound, idsFound, ts1, mysql, tstart, tend , cReader->getSearchResponseFormat());

    //unsigned idsFound = exactQueryResults->getNumberOfResults();
    //unsigned resultsToRetrieve = idsFound - offset;
    //printResults(conn, request_info, searchType, cReader, exactQueryResults,
    //                offset, resultsToRetrieve, resultsToRetrieve, ts1, mysql, tstart, tend , cReader->getSearchResponseFormat());

    delete exactQueryResults;
    //mysql_close(&mysql);

    return true;
}

//Stress Test
bool pingNormalQuery(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits ,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = numberofHits;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(), queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    //bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    bool returnvalue = true;

    queryResults->printStats();
    printResults(queryResults);

    delete queryResults;
    delete query;
    return returnvalue;
}

bool checkResults(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs)
{
    bool returnvalue = false;

    if (numberofHits != queryResults->getNumberOfResults()) {
        Logger::debug("%u | %d", numberofHits, queryResults->getNumberOfResults());
        for (unsigned resultCounter = 0;
                        resultCounter < queryResults->getNumberOfResults(); resultCounter++ ) {
                Logger::debug("%u", (unsigned)atoi(queryResults->getRecordId(resultCounter).c_str()));
        }
        return false;
    } else {
        returnvalue = true;
        for (unsigned resultCounter = 0;
                resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
        {
            vector<string> matchingKeywords;
            vector<unsigned> editDistances;
            vector<string>::iterator it1;
            vector<unsigned>::iterator it2;

            queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
            queryResults->getEditDistances(resultCounter, editDistances);

            if ( (unsigned)atoi(queryResults->getRecordId(resultCounter).c_str()) == recordIDs[resultCounter])
            {
                returnvalue = true;
            }
            else
            {
            	cout<<atoi(queryResults->getRecordId(resultCounter).c_str())<<endl;
                return false;
            }
        }
    }
    return returnvalue;
}

bool checkOutput(QueryResults *queryResults, unsigned numberofHits, bool isStemmed)
{
    //cout << numberofHits << " | " << queryResults->getNumberOfResults() << endl;

    if (numberofHits != queryResults->getNumberOfResults())
    {
        return false;
    }

    else
    {
        for (unsigned resultCounter = 0;
                resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
        {
            vector<string> matchingKeywords;
            vector<unsigned> editDistances;
            vector<string>::iterator it1;
            vector<unsigned>::iterator it2;
            queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
            queryResults->getEditDistances(resultCounter, editDistances);
            for(it1=matchingKeywords.begin();it1!=matchingKeywords.end();it1++)
            {
                 if(*it1 == "STEM")
                 {
                     return (isStemmed & true);
                 }
                 //cout<<"matching keyword: "<<"["<<*it1<<"]"<<endl;
            }
        }
    }

    return (!isStemmed);
}



bool checkResults(QueryResults *queryResults, unsigned numberOfHits ,unsigned recordID)
{
    vector<unsigned> rIDList;
    rIDList.push_back(recordID);
    return checkResults(queryResults, numberOfHits, rIDList);
}

// For Debugging while constructing test cases;
bool checkResults_DUMMY(QueryResults *queryResults, unsigned numberofHits ,const vector<unsigned> &recordIDs)
{
    bool returnvalue = true;
    if (checkResults(queryResults, numberofHits, recordIDs) == false)
    {
        cout << numberofHits << " |===| " << queryResults->getNumberOfResults() << endl;
        //if (numberofHits != queryResults->getNumberOfResults())
        //{
        //return false;
        //}
        //else
        //{
        //returnvalue = true;
        for (unsigned resultCounter = 0;
                resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
        {
            vector<string> matchingKeywords;
            vector<unsigned> editDistances;

            queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
            queryResults->getEditDistances(resultCounter, editDistances);

            if (queryResults->getNumberOfResults() < resultCounter)
            {
                cout << "[" << resultCounter << "]" << endl;
            }
            else
            {
                cout << "[" << resultCounter << "]" << queryResults->getRecordId(resultCounter) << "[" << queryResults->getResultScoreString(resultCounter) << "]" << endl;
                if ( (unsigned)atoi(queryResults->getRecordId(resultCounter).c_str()) == recordIDs[resultCounter])
                //if (queryResults->getRecordId(resultCounter) == recordIDs[resultCounter])
                {
                    //    returnvalue = true;
                }
                else
                {
                    //    return false;
                }
            }
        }
    }
    return returnvalue;
}

bool pingGetAllResultsQuery(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation, int attributeIdToSort = -1)
{
    QueryEvaluatorInternal * queryEvaluatorInternal = queryEvaluator->impl;
    Query *query = new Query(srch2::instantsearch::SearchTypeGetAllResultsQuery);
    parseExactPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);

    srch2::httpwrapper::SortFilterEvaluator * eval =
            new srch2::httpwrapper::SortFilterEvaluator();
	eval->order = srch2::instantsearch::SortOrderDescending;
    const std::map<std::string , unsigned> * nonSearchableAttributes =
    		queryEvaluatorInternal->getSchema()->getRefiningAttributes();
    string attributeToSortName = "";
    for(std::map<std::string , unsigned>::const_iterator nSA = nonSearchableAttributes->begin();
    		nSA != nonSearchableAttributes->end() ; nSA++){
    	if(nSA->second == attributeIdToSort){
    		attributeToSortName = nSA->first;
    	}
    }
    eval->field.push_back(attributeToSortName);

    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(), queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeGetAllResultsQuery);
    logicalPlan->setPostProcessingInfo(new ResultsPostProcessingInfo());
    logicalPlan->getPostProcessingInfo()->setSortEvaluator(eval);
    queryEvaluator->search(logicalPlan , queryResults);

    QueryResultFactory * factory = new QueryResultFactory();
    //bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    printResults(queryResults);
    delete queryResults;
    delete query;
    return returnvalue;
}

void getGetAllResultsQueryResults(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, bool descending, vector<string> &recordIds,
		vector<unsigned> attributeIdToFilter, bool andOperation, int attributeIdToSort = -1)
{
    QueryEvaluatorInternal * queryEvaluatorInternal = queryEvaluator->impl;
    Query *query = new Query(srch2::instantsearch::SearchTypeGetAllResultsQuery);
    parseExactPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    
    ResultsPostProcessorPlan * plan = NULL;
    plan = new ResultsPostProcessorPlan();
    srch2::httpwrapper::SortFilterEvaluator * eval =
            new srch2::httpwrapper::SortFilterEvaluator();
    if (descending)
		eval->order = srch2::instantsearch::SortOrderDescending;
    else
		eval->order = srch2::instantsearch::SortOrderAscending;
    const std::map<std::string , unsigned> * nonSearchableAttributes =
    		queryEvaluatorInternal->getSchema()->getRefiningAttributes();
    string attributeToSortName = "";
    for(std::map<std::string , unsigned>::const_iterator nSA = nonSearchableAttributes->begin();
    		nSA != nonSearchableAttributes->end() ; nSA++){
    	if(nSA->second == attributeIdToSort){
    		attributeToSortName = nSA->first;
    	}
    }
    eval->field.push_back(attributeToSortName);

    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(), queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeGetAllResultsQuery);
    logicalPlan->setPostProcessingInfo(new ResultsPostProcessingInfo());
    logicalPlan->getPostProcessingInfo()->setSortEvaluator(eval);
    queryEvaluator->search(logicalPlan , queryResults);

    QueryResultFactory * factory = new QueryResultFactory();

    for (unsigned resultCounter = 0; resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
        recordIds.push_back(queryResults->getRecordId(resultCounter));
    printResults(queryResults);

    delete queryResults;
    delete query;
}

//Test the ActiveNodeSet cache
bool pingCache1(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString)
{
    bool returnValue = false;

    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults =  new QueryResults(new QueryResultFactory(), queryEvaluator, query);

    QueryResultsInternal *q = queryResults->impl;
    QueryEvaluatorInternal * searcher = queryEvaluator->impl;
    if( q->checkCacheHit(searcher,query) )
    {
        returnValue = true;
    }

    delete queryResults;
    delete query;
    return returnValue;
}

//Test the ConjunctiveResults cache
bool pingCache2(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);
    int resultCount = 10;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    QueryEvaluatorInternal * searcher = queryEvaluator->impl;
    bool returnValue = searcher->cacheHit(query);

    // To put the queryResult in the cache
    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);

    delete queryResults;
    delete query;
    return returnValue;
}

bool ping_DUMMY(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    QueryEvaluatorInternal * searcher = queryEvaluator->impl;
    searcher->cacheHit(query);

    // To put the queryResult in the cache
    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnValue =  checkResults_DUMMY(queryResults, numberofHits, recordIDs);

    //printResults(queryResults);
    delete queryResults;
    delete query;
    return returnValue;
}

bool ping_DUMMY(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString,
		unsigned numberofHits , unsigned recordID ,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<unsigned> rIDList;
    rIDList.push_back(recordID);
    return ping_DUMMY(analyzer,queryEvaluator,queryString,numberofHits,rIDList,attributeIdToFilter, andOperation);
}


bool ping(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString,
		unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

bool ping(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString,
		unsigned numberofHits , unsigned recordID , vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<unsigned> rIDList;
    rIDList.push_back(recordID);
    return ping(analyzer,queryEvaluator,queryString,numberofHits,rIDList,attributeIdToFilter,andOperation);
}

bool pingExactPrefix(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

bool pingFuzzyPrefix(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

bool pingExactComplete(const Analyzer *analyzer, QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactCompleteQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

bool pingFuzzyComplete(const Analyzer *analyzer,  QueryEvaluator *queryEvaluator,
		string queryString, unsigned numberofHits , const vector<unsigned> &recordIDs,
		vector<unsigned> attributeIdToFilter, bool andOperation)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyCompleteQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

void parseEdQuery(const Analyzer *analyzer, Query *query, string queryString, vector<unsigned> attributeIdToFilter,
		bool andOperation, unsigned ed = 1)
{
    vector<AnalyzedTermInfo> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_COMPLETE;
        Term *term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, ed);
        term->addAttributesToFilter(attributeIdToFilter, andOperation);
        //query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }
    //cout << endl;
    queryKeywords.clear();
}

bool pingEd(const Analyzer *analyzer,  QueryEvaluator *queryEvaluator, string queryString,
		unsigned numberofHits , const vector<unsigned> &recordIDs, vector<unsigned> attributeIdToFilter,
		bool andOperation=true)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseEdQuery(analyzer, query, queryString, attributeIdToFilter, andOperation);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    queryResults->printStats();
    delete queryResults;
    delete query;
    return returnvalue;
}

bool pingEd(const Analyzer *analyzer,  QueryEvaluator *queryEvaluator, string queryString,
		unsigned numberofHits , unsigned recordID , vector<unsigned> attributeIdToFilter, bool andOperation)
{
    vector<unsigned> rIDList;
    rIDList.push_back(recordID);
    return pingEd(analyzer,queryEvaluator,queryString,numberofHits,rIDList,attributeIdToFilter, andOperation);
}

// fuzzy query by default
float pingToGetTopScore(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, 10, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    //printResults(queryResults);

    float resultValue = queryResults->getResultScore(0).getFloatTypedValue();
    delete queryResults;
    delete query;
    return resultValue;
}

int pingForScalabilityTest(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, const string &queryString, unsigned ed)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseFuzzyQueryWithEdSet(analyzer, query, queryString, ed);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    //bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    //cout << "Number of results: " << queryResults->getNumberOfResults() << endl;
    int returnValue =  queryResults->getNumberOfResults();
    delete queryResults;
    delete query;
    return returnValue;
}

void pingDummyStressTest(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString, unsigned numberofHits = 10)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);

    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);


    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    //bool returnvalue =  checkResults(queryResults, numberofHits, recordIDs);
    //printResults(queryResults);
    //cout << "Number of results: " << queryResults->getNumberOfResults() << endl;
    //bool returnvalue =  queryResults->getNumberOfResults()>0;
    delete queryResults;
    delete query;
    return;
}

bool topK1ConsistentWithTopK2(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString, const unsigned k1, const unsigned k2)
{
    if(k1 > k2)
    {
        cout << "ERROR: K1 should be no greater than K2." << endl;
        return false;
    }

    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);

    //cout << "[" << queryString << "]" << endl;

    QueryResults *queryResultsK1 = new QueryResults(new QueryResultFactory(),queryEvaluator, query);
    QueryResults *queryResultsK2 = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, k1, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResultsK1);
    logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, k2, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResultsK2);

    //printResults(queryResultsK1);
    //printResults(queryResultsK2);

    unsigned k1ResultNum =  queryResultsK1->getNumberOfResults();
    for(unsigned i = 0; i < k1ResultNum; i++)
    {
        if(queryResultsK1->getRecordId(i) != queryResultsK2->getRecordId(i))
            return false;
    }

    delete queryResultsK1;
    delete queryResultsK2;
    delete query;

    return true;
}

bool existsInTopK(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString, string primaryKey, const unsigned k)
{

    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);

    //cout << "[" << queryString << "]" << endl;

    QueryResults *queryResultsK =new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, k, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResultsK);

    //printResults(queryResultsK1);

    bool inTopK = false;

    unsigned kResultNum =  queryResultsK->getNumberOfResults();
    for(unsigned i = 0; i < kResultNum; i++)
    {
        if(queryResultsK->getRecordId(i) == primaryKey)
        {
            inTopK = true;
            break;
        }
    }

    delete queryResultsK;
    delete query;

    return inTopK;
}

unsigned pingExactTest(const Analyzer *analyzer, QueryEvaluator *queryEvaluator, string queryString)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeTopKQuery);
    parseExactPrefixQuery(analyzer, query, queryString, vector<unsigned>(), true);
    int resultCount = 10;

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, resultCount, false, srch2::instantsearch::SearchTypeTopKQuery);
    queryEvaluator->search(logicalPlan , queryResults);
    unsigned returnvalue =  queryResults->getNumberOfResults();
    //printResults(queryResults);
    delete queryResults;
    delete query;
    return returnvalue;
}


void csvline_populate(vector<string> &record, const string& line, char delimiter)
{
    int linepos=0;
    int inquotes=false;
    char c;
    //int i;
    int linemax=line.length();
    string curstring;
    record.clear();

    while(line[linepos]!=0 && linepos < linemax)
    {

        c = line[linepos];

        if (!inquotes && curstring.length()==0 && c=='"')
        {
            //beginquotechar
            inquotes=true;
        }
        else if (inquotes && c=='"')
        {
            //quotechar
            if ( (linepos+1 <linemax) && (line[linepos+1]=='"') )
            {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            }
            else
            {
                //endquotechar
                inquotes=false;
            }
        }
        else if (!inquotes && c==delimiter)
        {
            //end of field
            record.push_back( curstring );
            curstring="";
        }
        else if (!inquotes && (c=='\r' || c=='\n') )
        {
            record.push_back( curstring );
            return;
        }
        else
        {
            curstring.push_back(c);
        }
        linepos++;
    }
    record.push_back( curstring );
    return;
}

Analyzer * getAnalyzer()
{
    return new Analyzer(NULL, NULL, NULL, NULL, "", srch2is::STANDARD_ANALYZER);
}

#endif /* __INTEGRATIONTESTHELPER_H__ */
