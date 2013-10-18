//
////$Id: IndexSearcherInternal_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
//
#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "util/Assert.h"
#include "analyzer/AnalyzerInternal.h"
#include "util/Logger.h"
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/SortFilter.h>
#include <wrapper/SortFilterEvaluator.h>
#include <instantsearch/NonSearchableAttributeExpressionFilter.h>
#include <instantsearch/FacetedSearchFilter.h>
//
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>
//
using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie Trie_Internal;

const char* INDEX_DIR = ".";

using srch2::util::Logger;

QueryResults * applyFilter(QueryResults * initialQueryResults,
        IndexSearcher * indexSearcher, Query * query,
        ResultsPostProcessorPlan * plan) {

//    ResultsPostProcessor postProcessor(indexSearcher);

    QueryResults * finalQueryResults = new QueryResults(
            new QueryResultFactory(), indexSearcher, query);

    plan->beginIteration();

    // short circuit in case the plan doesn't have any filters in it.
    // if no plan is set in Query or there is no filter in it,
    // then there is no post processing so just mirror the results
    if (plan == NULL) {
        finalQueryResults->copyForPostProcessing(initialQueryResults);
        return finalQueryResults;
    }

    plan->beginIteration();
    if (!plan->hasMoreFilters()) {
        finalQueryResults->copyForPostProcessing(initialQueryResults);
        plan->closeIteration();
        return finalQueryResults;
    }

    // iterating on filters and applying them on list of results
    while (plan->hasMoreFilters()) {
        ResultsPostProcessorFilter * filter = plan->nextFilter();

        // clear the output to be ready to accept the result of the filter
        finalQueryResults->clear();
        // apply the filter on the input and put the results in output
        filter->doFilter(indexSearcher, query, initialQueryResults,
                finalQueryResults);
        // if there is going to be other filters, chain the output to the input
        if (plan->hasMoreFilters()) {
            initialQueryResults->copyForPostProcessing(finalQueryResults);
        }

    }
    plan->closeIteration();

    return finalQueryResults;
}

void addRecords() {
    /// Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text
    schema->setNonSearchableAttribute("citation", ATTRIBUTE_TYPE_UNSIGNED, "0");
    schema->setNonSearchableAttribute("price", ATTRIBUTE_TYPE_FLOAT, "1.25");
    schema->setNonSearchableAttribute("class", ATTRIBUTE_TYPE_TEXT, "Z");

    Record *record = new Record(schema);
    Analyzer *analyzer = new Analyzer(
            srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
            "", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");


    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData(NULL,
            mergeEveryNSeconds, mergeEveryMWrites,
            updateHistogramEveryPMerges, updateHistogramEveryQWrites,
            INDEX_DIR, "");
    srch2is::Indexer *index = srch2is::Indexer::create(indexMetaData, analyzer,
            schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors",
            "Tom Smith and Jack Lennon zzzzzz");
    record->setSearchableAttributeValue("article_title",
            "come Yesterday Once More");
    record->setNonSearchableAttributeValue("citation", "1");
    record->setNonSearchableAttributeValue("price", "10.34");
    record->setNonSearchableAttributeValue("class", "A");
    record->setRecordBoost(10);
    record->setInMemoryData("test string");
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris zzzzzz");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setNonSearchableAttributeValue("citation", "2");
    record->setNonSearchableAttributeValue("price", "9.34");
    record->setNonSearchableAttributeValue("class", "A");
    record->setRecordBoost(20);
    record->setInMemoryData("test string");
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd zzzzzz");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setNonSearchableAttributeValue("citation", "3");
    record->setNonSearchableAttributeValue("price", "8.34");
    record->setNonSearchableAttributeValue("class", "B");
    record->setRecordBoost(30);
    record->setInMemoryData("test string");
    index->addRecord(record,analyzer);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp zzzzzz");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setNonSearchableAttributeValue("citation", "4");
    record->setNonSearchableAttributeValue("price", "7.34");
    record->setNonSearchableAttributeValue("class", "B");
    record->setRecordBoost(40);
    record->setInMemoryData("test string");
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi zzzzzz");
    record->setSearchableAttributeValue(1,
            "Shinesyponzi on Wish you were here");
    record->setNonSearchableAttributeValue("citation", "5");
    record->setNonSearchableAttributeValue("price", "6.34");
    record->setNonSearchableAttributeValue("class", "C");
    record->setRecordBoost(50);
    record->setInMemoryData("test string");
    index->addRecord(record,analyzer);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink zzzzzz");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setNonSearchableAttributeValue("citation", "6");
    record->setNonSearchableAttributeValue("price", "5.34");
    record->setNonSearchableAttributeValue("class", "C");
    record->setRecordBoost(60);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin zzzzzz");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setNonSearchableAttributeValue("citation", "7");
    record->setNonSearchableAttributeValue("price", "4.34");
    record->setNonSearchableAttributeValue("class", "D");
    record->setRecordBoost(80);
    index->addRecord(record, analyzer);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix zzzzzz");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setNonSearchableAttributeValue("citation", "8");
    record->setNonSearchableAttributeValue("price", "3.34");
    record->setNonSearchableAttributeValue("class", "E");
    record->setRecordBoost(90);
    index->addRecord(record, analyzer);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

bool checkResults(QueryResults *queryResults, vector<unsigned> *resultSet)

{
    if (queryResults->getNumberOfResults() != resultSet->size())
        return false;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults();
            resultCounter++) {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
        queryResults->getEditDistances(resultCounter, editDistances);

//        Logger::info("\nResult-(%d) RecordId: %s\tScore: %s",
//        		resultCounter,
//        		queryResults->getRecordId(resultCounter).c_str() ,
//        		queryResults->getResultScoreString(resultCounter).c_str());
//
//        Logger::info("\nMatching Keywords:");
//
//        unsigned counter = 0;
//        for(vector<string>::iterator iter = matchingKeywords.begin(); iter != matchingKeywords.end(); iter++, counter++ )
//        {
//        	Logger::info("\t%s %d", (*iter).c_str(), editDistances.at(counter));
//        }

        if (resultSet->at(resultCounter)
                != atoi(queryResults->getRecordId(resultCounter).c_str())) {

            Logger::info(
                    "\n=====================================================");
            for (unsigned resultCounter = 0;
                    resultCounter < queryResults->getNumberOfResults();
                    resultCounter++) {
                Logger::info("\nResult-(%d) RecordId: %s", resultCounter,
                        queryResults->getRecordId(resultCounter).c_str());
            }
            return false;
        }

    }
    return true;
}
bool checkFacetResultsHelper(
        const std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > > * facetResults,
        const std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > > * facetResultsForTest) {

    if (facetResultsForTest->size() != facetResults->size()) {

        return false;
    }
    for (std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
            facetResults->begin(); attrIter != facetResults->end();
            ++attrIter) {
        const std::vector<std::pair<std::string, float> > & toTest =
                facetResultsForTest->at(attrIter->first).second;
        const std::vector<std::pair<std::string, float> > & correct =
                facetResults->at(attrIter->first).second;

        bool flag = true;
        for (std::vector<std::pair<std::string, float> >::const_iterator correctPair =
                correct.begin(); correctPair != correct.end(); ++correctPair) {
            for (std::vector<std::pair<std::string, float> >::const_iterator toTestPair =
                    correct.begin(); toTestPair != correct.end();
                    ++toTestPair) {
                if (correctPair->first.compare(toTestPair->first) == 0) {
                    if (correctPair->second != toTestPair->second) {
                        flag = false;
                        break;
                    } else {
                        break;
                    }

                }
            }
            if (flag == false)
                break;
        }

        if (flag)
            continue;

        return false;

    }
    return true;
}

void printFacetResults(
        const std::map<std::string, std::pair<FacetType , std::vector<std::pair<std::string, float> > > > * facetResults) {
    Logger::info("Facet results : \n");
    for (std::map<std::string, std::pair<FacetType , std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
            facetResults->begin(); attrIter != facetResults->end();
            ++attrIter) {
        Logger::info("Attribute : %s\n", attrIter->first.c_str());
        for (std::vector<std::pair<std::string, float> >::const_iterator aggregationResult =
                attrIter->second.second.begin();
                aggregationResult != attrIter->second.second.end();
                ++aggregationResult) {
            Logger::info("\t%s : %.3f \n", aggregationResult->first.c_str(),
                    aggregationResult->second);
        }
        Logger::info("\n ======================================= \n");
    }
}

bool checkFacetedFilterResults(QueryResults * queryResults,
        const std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > > * facetResults) {
    const std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > > * facetResultsForTest =
            queryResults->getFacetResults();

    bool match = checkFacetResultsHelper(facetResults, facetResultsForTest);

    if (!match) {
        Logger::info("Facet results : \n");
        for (std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
                facetResultsForTest->begin();
                attrIter != facetResultsForTest->end(); ++attrIter) {
            Logger::info("Attribute : %s\n", attrIter->first.c_str());
            for (std::vector<std::pair<std::string, float> >::const_iterator aggregationResult =
                    attrIter->second.second.begin();
                    aggregationResult != attrIter->second.second.end();
                    ++aggregationResult) {
                Logger::info("\t%s : %.3f \n", aggregationResult->first.c_str(),
                        aggregationResult->second);
            }
            Logger::info("\n ======================================= \n");
        }
        Logger::info("Correct Facet results : \n");
        for (std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
                facetResults->begin(); attrIter != facetResults->end();
                ++attrIter) {
            Logger::info("Attribute : %s\n", attrIter->first.c_str());
            for (std::vector<std::pair<std::string, float> >::const_iterator aggregationResult =
                    attrIter->second.second.begin();
                    aggregationResult != attrIter->second.second.end();
                    ++aggregationResult) {
                Logger::info("\t%s : %.3f \n", aggregationResult->first.c_str(),
                        aggregationResult->second);
            }
            Logger::info("\n ======================================= \n");
        }
        return match;
    }

    match = checkFacetResultsHelper(facetResultsForTest, facetResults);

    if (!match) {
        Logger::info("Facet results : \n");
        for (std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
                facetResultsForTest->begin();
                attrIter != facetResultsForTest->end(); ++attrIter) {
            Logger::info("Attribute : %s\n", attrIter->first.c_str());
            for (std::vector<std::pair<std::string, float> >::const_iterator aggregationResult =
                    attrIter->second.second.begin();
                    aggregationResult != attrIter->second.second.end();
                    ++aggregationResult) {
                Logger::info("\t%s : %.3f \n", aggregationResult->first.c_str(),
                        aggregationResult->second);
            }
            Logger::info("\n ======================================= \n");
        }
        Logger::info("Correct Facet results : \n");
        for (std::map<std::string, std::pair<FacetType ,  std::vector<std::pair<std::string, float> > > >::const_iterator attrIter =
                facetResults->begin(); attrIter != facetResults->end();
                ++attrIter) {
            Logger::info("Attribute : %s\n", attrIter->first.c_str());
            for (std::vector<std::pair<std::string, float> >::const_iterator aggregationResult =
                    attrIter->second.second.begin();
                    aggregationResult != attrIter->second.second.end();
                    ++aggregationResult) {
                Logger::info("\t%s : %.3f \n", aggregationResult->first.c_str(),
                        aggregationResult->second);
            }
            Logger::info("\n ======================================= \n");
        }
        return match;
    }

    return match;

}

void Test_Sort_Filter(IndexSearcherInternal *indexSearcherInternal) {

    std::vector<unsigned> resultSet0, resultSet01, resultSet02, resultSet03,
            resultSet1, resultSet2, resultSetEmpty;

    // query = "pionn", sort: citation
    resultSet0.push_back(1003);
    resultSet0.push_back(1005);
    resultSet0.push_back(1006);
    resultSet0.push_back(1007);

    // query = "pionn", sort: price
    resultSet01.push_back(1007);
    resultSet01.push_back(1006);
    resultSet01.push_back(1005);
    resultSet01.push_back(1003);

    // query = "pionn", sort: class
    resultSet02.push_back(1003);
    resultSet02.push_back(1005);
    resultSet02.push_back(1006);
    resultSet02.push_back(1007);

    // query = "pionn", sort: class,citation
    resultSet03.push_back(1003);
    resultSet03.push_back(1005);
    resultSet03.push_back(1006);
    resultSet03.push_back(1007);

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = { "pionn", "fllio", "shiii" };

    Logger::info("***PREFIX FUZZY***\nQuery:");
    TermType type = srch2::instantsearch::TERM_TYPE_PREFIX;
    Logger::info(keywords[0].c_str());

    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);

    ResultsPostProcessorPlan * plan = NULL;
    plan = new ResultsPostProcessorPlan();
    SortFilter * sortFilter = new SortFilter();
    srch2::httpwrapper::SortFilterEvaluator * eval =
            new srch2::httpwrapper::SortFilterEvaluator();
    sortFilter->evaluator = eval;
    eval->order = srch2::instantsearch::SortOrderAscending;
    eval->field.push_back("citation");

    plan->addFilterToPlan(sortFilter);

    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,
            indexSearcherInternal, query, plan);

    bool valid = checkResults(queryResultsAfterFilter, &resultSet0);
    ASSERT(valid);

    ////////////////////////////////////////////////////

    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    sortFilter = new SortFilter();
    eval = new srch2::httpwrapper::SortFilterEvaluator();
    sortFilter->evaluator = eval;
    eval->order = srch2::instantsearch::SortOrderAscending;
    eval->field.push_back("price");

    plan->addFilterToPlan(sortFilter);

    queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);

    valid = checkResults(queryResultsAfterFilter, &resultSet01);
    ASSERT(valid);

    ////////////////////////////////////////////////////////

    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    sortFilter = new SortFilter();
    eval = new srch2::httpwrapper::SortFilterEvaluator();
    sortFilter->evaluator = eval;
    eval->order = srch2::instantsearch::SortOrderAscending;
    eval->field.push_back("class");

    plan->addFilterToPlan(sortFilter);

    queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);

    valid = checkResults(queryResultsAfterFilter, &resultSet02);
    ASSERT(valid);

    ///////////////////////////////////////////////////////////

    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    sortFilter = new SortFilter();
    eval = new srch2::httpwrapper::SortFilterEvaluator();
    sortFilter->evaluator = eval;
    eval->order = srch2::instantsearch::SortOrderAscending;
    eval->field.push_back("class");
    eval->field.push_back("citation");

    plan->addFilterToPlan(sortFilter);

    queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);

    valid = checkResults(queryResultsAfterFilter, &resultSet03);
    ASSERT(valid);

    /////////////////////////////////////////////////////////////

    // test with empty results
    type = srch2is::TERM_TYPE_PREFIX;

    Logger::info("keywordwhichisnotinresults");

    Term *term1 = FuzzyTerm::create("keywordwhichisnotinresults", type, 1, 1,
            2);
    query->add(term1);

    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    sortFilter = new SortFilter();
    eval = new srch2::httpwrapper::SortFilterEvaluator();
    sortFilter->evaluator = eval;
    eval->order = srch2::instantsearch::SortOrderAscending;
    eval->field.push_back("price");

    plan->addFilterToPlan(sortFilter);

    queryResults = new QueryResults(new QueryResultFactory(),
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);

    valid = checkResults(queryResultsAfterFilter, &resultSetEmpty);
    ASSERT(valid);

    delete query;
    delete queryResults;
//    delete queryResults1;
//    delete queryResults2;

}

//void Test_Expression_Filter(IndexSearcherInternal *indexSearcherInternal){
//
//    std::vector<unsigned> resultSet0 , resultSet01, resultSet02, resultSet03,  resultSet1, resultSet2, resultSetEmpty;
//
//    // query = "pionn", citation < 7
//    resultSet0.push_back(1003);
//    resultSet0.push_back(1005);
//    resultSet0.push_back(1006);
//
//    // query = "pionn", price < 8
//    resultSet01.push_back(1005);
//    resultSet01.push_back(1006);
//    resultSet01.push_back(1007);
//
//    // query = "pionn", class < D
//    resultSet02.push_back(1003);
//    resultSet02.push_back(1005);
//    resultSet02.push_back(1006);
//
//
//
//
//    int resultCount = 10;
//    // create a query
//    Query *query = new Query(srch2is::TopKQuery);
//    string keywords[3] = {
//            "pionn","fllio","shiii"
//    };
//
//
//
//
//
//
//
//    cout<<"\n***PREFIX FUZZY***\nQuery:";
//
//    // query = "pionn", citation < 7
//    TermType type = PREFIX;
//    cout<<keywords[0]<< "\n";
//    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
//    query->add(term0);
//
//
//	ResultsPostProcessorPlan * plan = NULL;
//	plan = new ResultsPostProcessorPlan();
//	NonSearchableAttributeExpressionFilter * expressionFilter = new NonSearchableAttributeExpressionFilter();
//	// these two line must be changed by setting something like an expression folder in filter
//	expressionFilter->attributeName = "citation";
//	Score se7en;
//	se7en.setScore((unsigned)7);
//	expressionFilter->attributeValue = se7en;
//
//	plan->addFilterToPlan(expressionFilter);
//	query->setPostProcessingPlan(plan);
//
//
//
//
//    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
//    indexSearcherInternal->search(query, queryResults, resultCount);
//
//
//    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
//
//
//    ASSERT(checkResults(queryResultsAfterFilter, &resultSet0));
//
//    ////////////////////////////////////////////////////
//    // query = "pionn", price < 8
//	plan = NULL;
//	plan = new ResultsPostProcessorPlan();
//	expressionFilter = new NonSearchableAttributeExpressionFilter();
//	// these two line must be changed by setting something like an expression folder in filter
//	expressionFilter->attributeName = "price";
//	Score eig8t;
//	eig8t.setScore((float)8);
//	expressionFilter->attributeValue = eig8t;
//
//	plan->addFilterToPlan(expressionFilter);
//	query->setPostProcessingPlan(plan);
//
//
//
//
//    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
//    indexSearcherInternal->search(query, queryResults, resultCount);
//
//
//    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
//
//    ASSERT(checkResults(queryResultsAfterFilter, &resultSet01));
//
//
//    ////////////////////////////////////////////////////////
//
//    // query = "pionn", class < D
//	plan = NULL;
//	plan = new ResultsPostProcessorPlan();
//	expressionFilter = new NonSearchableAttributeExpressionFilter();
//	// these two line must be changed by setting something like an expression folder in filter
//	expressionFilter->attributeName = "class";
//	Score D;
//	D.setScore("D");
//	expressionFilter->attributeValue = D;
//
//	plan->addFilterToPlan(expressionFilter);
//	query->setPostProcessingPlan(plan);
//
//
//
//    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
//    indexSearcherInternal->search(query, queryResults, resultCount);
//
//    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
//
//    ASSERT(checkResults(queryResultsAfterFilter, &resultSet02));
//
//    ///////////////////////////////////////////////////////////
//
//
//    // test with empty results
//    type = PREFIX;
//    cout<<"keywordwhichisnotinresults"<< "\n";
//
//
//    Term *term1 = FuzzyTerm::create("keywordwhichisnotinresults", type, 1, 1, 2);
//    query->add(term1);
//
//
//	plan = NULL;
//	plan = new ResultsPostProcessorPlan();
//	expressionFilter = new NonSearchableAttributeExpressionFilter();
//	// these two line must be changed by setting something like an expression folder in filter
//	expressionFilter->attributeName = "citation";
//	se7en.setScore((unsigned)7);
//	expressionFilter->attributeValue = se7en;
//
//	plan->addFilterToPlan(expressionFilter);
//	query->setPostProcessingPlan(plan);
//
//
//
//
//    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
//    indexSearcherInternal->search(query, queryResults, resultCount);
//
//
//    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
//
//    ASSERT(checkResults(queryResultsAfterFilter, &resultSetEmpty));
//
//
//
//    delete query;
//    delete queryResults;
////    delete queryResults1;
////    delete queryResults2;
//
//}
//
//
void Test_FacetedSearch_Filter(IndexSearcherInternal *indexSearcherInternal) {

    std::vector<unsigned> resultSet0, resultSet01, resultSet02, resultSet03,
            resultSet1, resultSet2, resultSetEmpty;

    // query = "zzzzzzz", facet: class<A,B,C,D,E>
    resultSet0.push_back(1001);
    resultSet0.push_back(1002);
    resultSet0.push_back(1003);
    resultSet0.push_back(1004);
    resultSet0.push_back(1005);
    resultSet0.push_back(1006);
    resultSet0.push_back(1007);
    resultSet0.push_back(1008);

    std::map<std::string, std::pair<FacetType , std::vector<std::pair<std::string, float> > > > facetResults; // class , simple
    std::vector<std::pair<std::string, float> > classFacetResults;
    classFacetResults.push_back(std::make_pair("A", 2));
    classFacetResults.push_back(std::make_pair("B", 2));
    classFacetResults.push_back(std::make_pair("C", 2));
    classFacetResults.push_back(std::make_pair("D", 1));
    classFacetResults.push_back(std::make_pair("E", 1));
    facetResults["class"] = std::make_pair(FacetTypeCategorical , classFacetResults);
    //////////////////////////////////

    std::map<std::string, std::pair<FacetType , std::vector<std::pair<std::string, float> > > > facetResults2; //class , simple & citation : 1,5 range
    std::vector<std::pair<std::string, float> > classFacetResults2;
    classFacetResults2.push_back(std::make_pair("A", 2));
    classFacetResults2.push_back(std::make_pair("B", 2));
    classFacetResults2.push_back(std::make_pair("C", 2));
    classFacetResults2.push_back(std::make_pair("D", 1));
    classFacetResults2.push_back(std::make_pair("E", 1));
    facetResults2["class"] = std::make_pair(FacetTypeCategorical , classFacetResults2);

    std::vector<std::pair<std::string, float> > citationFacetResults2;
    citationFacetResults2.push_back(std::make_pair("0", 0));
    citationFacetResults2.push_back(std::make_pair("1", 4));
    citationFacetResults2.push_back(std::make_pair("5", 4));
    facetResults2["citation"] = std::make_pair(FacetTypeRange , citationFacetResults2);
    //////////////////////////////////

    std::map<std::string, std::pair<FacetType , std::vector<std::pair<std::string, float> > > > facetResults3; //price : 5,7 , range & class , simple & citation : 1 , range
    std::vector<std::pair<std::string, float> > priceFacetResults3;
    priceFacetResults3.push_back(std::make_pair("0", 2)); // < 5 : 3.34,4.34
    priceFacetResults3.push_back(std::make_pair("5", 2)); // >=5 : 5.34,6.34
    priceFacetResults3.push_back(std::make_pair("7", 4)); // >=7 : 7.34,8.34,9.34,10.34
    facetResults3["price"] = std::make_pair(FacetTypeRange , priceFacetResults3);
    std::vector<std::pair<std::string, float> > classFacetResults3;
    classFacetResults3.push_back(std::make_pair("A", 2));
    classFacetResults3.push_back(std::make_pair("B", 2));
    classFacetResults3.push_back(std::make_pair("C", 2));
    classFacetResults3.push_back(std::make_pair("D", 1));
    classFacetResults3.push_back(std::make_pair("E", 1));
    facetResults3["class"] = std::make_pair(FacetTypeCategorical , classFacetResults3);
    std::vector<std::pair<std::string, float> > citationFacetResults3;
    citationFacetResults3.push_back(std::make_pair("0", 0));
    citationFacetResults3.push_back(std::make_pair("1", 8));
    facetResults3["citation"] = std::make_pair(FacetTypeRange , citationFacetResults3);
    //////////////////////////////////

    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[1] = { "zzzzzzz" };

    Logger::info("\n***PREFIX FUZZY***\nQuery:");

    // query = "zzzzzzz",
    TermType type = TERM_TYPE_PREFIX;
    Logger::info(keywords[0].c_str());
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);

    // facet: // class , simple
    ResultsPostProcessorPlan * plan = NULL;
    plan = new ResultsPostProcessorPlan();
    FacetedSearchFilter * facetFilter = new FacetedSearchFilter();

    std::vector<FacetType> types;
    std::vector<std::string> fields;
    std::vector<std::string> rangeStarts;
    std::vector<std::string> rangeEnds;
    std::vector<std::string> rangeGaps;

    types.push_back(srch2::instantsearch::FacetTypeCategorical);
    fields.push_back("class");
    rangeStarts.push_back("");
    rangeEnds.push_back("");
    rangeGaps.push_back("");

    facetFilter->initialize(types, fields, rangeStarts, rangeEnds, rangeGaps);

    plan->addFilterToPlan(facetFilter);
    query->setPostProcessingPlan(plan);
    QueryResultFactory * factory = new QueryResultFactory();
    QueryResults *queryResults = new QueryResults(factory,
            indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,
            indexSearcherInternal, query, plan);
    bool valid = checkFacetedFilterResults(queryResultsAfterFilter,
            &facetResults);
    printFacetResults(queryResultsAfterFilter->getFacetResults());
    ASSERT(valid);

//    ASSERT(checkResults(queryResultsAfterFilter, &resultSet0));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;
    ////////////////////////////////////////////////////
    // facet: //class , simple & citation : 1,5 range

    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    facetFilter = new FacetedSearchFilter();

    types.clear();
    fields.clear();
    rangeStarts.clear();
    rangeEnds.clear();
    rangeGaps.clear();

    // class
    types.push_back(srch2::instantsearch::FacetTypeCategorical);
    fields.push_back("class");
    rangeStarts.push_back("");
    rangeEnds.push_back("");
    rangeGaps.push_back("");
    // citation
    types.push_back(srch2::instantsearch::FacetTypeRange);
    fields.push_back("citation");
    rangeStarts.push_back("1");
    rangeEnds.push_back("5");
    rangeGaps.push_back("4");

    facetFilter->initialize(types, fields, rangeStarts, rangeEnds, rangeGaps);

    plan->addFilterToPlan(facetFilter);
    query->setPostProcessingPlan(plan);
    factory = new QueryResultFactory();
    queryResults = new QueryResults(factory, indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);
    valid = checkFacetedFilterResults(queryResultsAfterFilter, &facetResults2);
    printFacetResults(queryResultsAfterFilter->getFacetResults());
    ASSERT(valid);
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;

    //////////////////////////////////
    ////price : 5,7 , range & class , simple & citation : 1 , range
    plan = NULL;
    plan = new ResultsPostProcessorPlan();
    facetFilter = new FacetedSearchFilter();

    types.clear();
    fields.clear();
    rangeStarts.clear();
    rangeEnds.clear();
    rangeGaps.clear();

    // price
    types.push_back(srch2::instantsearch::FacetTypeRange);
    fields.push_back("price");
    rangeStarts.push_back("5");
    rangeEnds.push_back("7");
    rangeGaps.push_back("2");
    // class
    types.push_back(srch2::instantsearch::FacetTypeCategorical);
    fields.push_back("class");
    rangeStarts.push_back("");
    rangeEnds.push_back("");
    rangeGaps.push_back("");
    // citation
    types.push_back(srch2::instantsearch::FacetTypeRange);
    fields.push_back("citation");
    rangeStarts.push_back("1");
    rangeEnds.push_back("1");
    rangeGaps.push_back("0");

    facetFilter->initialize(types, fields, rangeStarts, rangeEnds, rangeGaps);

    plan->addFilterToPlan(facetFilter);
    query->setPostProcessingPlan(plan);
    factory = new QueryResultFactory();
    queryResults = new QueryResults(factory, indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults, indexSearcherInternal,
            query, plan);
    ASSERT(checkFacetedFilterResults(queryResultsAfterFilter, &facetResults3));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;

}

void Searcher_Tests() {
    addRecords();

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData(
            new Cache(), mergeEveryNSeconds, mergeEveryMWrites,
            updateHistogramEveryPMerges , updateHistogramEveryQWrites,
            INDEX_DIR, "");

    Indexer* indexer = Indexer::load(indexMetaData);

    IndexSearcherInternal *indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));

    Logger::info("Test 1");
    Test_Sort_Filter(indexSearcherInternal);
//	std::cout << "test2" << std::endl;
//	Test_Expression_Filter(indexSearcherInternal);
    Logger::info("Test 3");
    Test_FacetedSearch_Filter(indexSearcherInternal);

    delete indexer;
    delete indexSearcherInternal;
}
int main(int argc, char *argv[]) {
    bool verbose = false;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }

    Searcher_Tests();

    Logger::info("Post Processing Filters Unit Tests: Passed\n");
    return 0;
}
//
