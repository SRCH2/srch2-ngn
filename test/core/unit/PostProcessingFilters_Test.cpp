
//$Id: IndexSearcherInternal_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include "operation/IndexSearcherInternal.h"
#include "operation/IndexerInternal.h"
#include "util/Assert.h"
#include "analyzer/AnalyzerInternal.h"

#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/RangeQueryFilter.h>
#include <instantsearch/SortFilter.h>
#include <instantsearch/NonSearchableAttributeExpressionFilter.h>
#include <instantsearch/FacetedSearchFilter.h>

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

typedef Trie Trie_Internal;

const char* INDEX_DIR = ".";

bool checkContainment(vector<string> &prefixVector, const string &prefix)
{
    for (unsigned i = 0; i < prefixVector.size(); i++)
    {
        if (prefixVector.at(i) == prefix)
            return true;
    }

    return false;
}



/**
 * Keyword: Record
 *   Pink:  2, 5, 6
 *   Pinksyponzi: 4
 *
 *   Floyd: 2, 6
 *   Floydsyponzi: 4
 *
 *   Shine: 2, 3
 *   Shinesyponzi: 4
 */

QueryResults *  applyFilter(QueryResults * initialQueryResults , IndexSearcher * indexSearcher , Query * query){

//    ResultsPostProcessor postProcessor(indexSearcher);

    QueryResults * finalQueryResults = new QueryResults(new QueryResultFactory(),indexSearcher,query);
//    postProcessor.runPlan(query, initialQueryResults , finalQueryResults);



    return finalQueryResults;
}



void addRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text
    schema->setNonSearchableAttribute("citation", UNSIGNED, "0" , true);
    schema->setNonSearchableAttribute("price" , FLOAT, "1.25", true);
    schema->setNonSearchableAttribute("class" , TEXT, "Z" , false);

    Record *record = new Record(schema);
    Analyzer *analyzer = srch2is::Analyzer::create(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( NULL, mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");
    srch2is::Indexer *index = srch2is::Indexer::create(indexMetaData, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon zzzzzz");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setNonSearchableAttributeValue("citation", "1");
    record->setNonSearchableAttributeValue("price" , "10.34");
    record->setNonSearchableAttributeValue("class" , "A");
    record->setRecordBoost(10);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(0, "George Harris zzzzzz");
    record->setSearchableAttributeValue(1, "Here comes the sun");
    record->setNonSearchableAttributeValue("citation", "2");
    record->setNonSearchableAttributeValue("price" , "9.34");
    record->setNonSearchableAttributeValue("class" , "A");
    record->setRecordBoost(20);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(0, "Pink Floyd zzzzzz");
    record->setSearchableAttributeValue(1, "Shine on you crazy diamond");
    record->setNonSearchableAttributeValue("citation", "3");
    record->setNonSearchableAttributeValue("price" , "8.34");
    record->setNonSearchableAttributeValue("class" , "B");
    record->setRecordBoost(30);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1004);
    record->setSearchableAttributeValue(0, "Uriah Hepp zzzzzz");
    record->setSearchableAttributeValue(1, "Come Shine away Melinda ");
    record->setNonSearchableAttributeValue("citation", "4");
    record->setNonSearchableAttributeValue("price" , "7.34");
    record->setNonSearchableAttributeValue("class" , "B");
    record->setRecordBoost(40);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1005);
    record->setSearchableAttributeValue(0, "Pinksyponzi Floydsyponzi zzzzzz");
    record->setSearchableAttributeValue(1, "Shinesyponzi on Wish you were here");
    record->setNonSearchableAttributeValue("citation", "5");
    record->setNonSearchableAttributeValue("price" , "6.34");
    record->setNonSearchableAttributeValue("class" , "C");
    record->setRecordBoost(50);
    record->setInMemoryData("test string");
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1006);
    record->setSearchableAttributeValue(0, "U2 2345 Pink zzzzzz");
    record->setSearchableAttributeValue(1, "with or without you");
    record->setNonSearchableAttributeValue("citation", "6");
    record->setNonSearchableAttributeValue("price" , "5.34");
    record->setNonSearchableAttributeValue("class" , "C");
    record->setRecordBoost(60);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1007);
    record->setSearchableAttributeValue(0, "Led Zepplelin zzzzzz");
    record->setSearchableAttributeValue(1, "Stairway to Heaven pink floyd");
    record->setNonSearchableAttributeValue("citation", "7");
    record->setNonSearchableAttributeValue("price" , "4.34");
    record->setNonSearchableAttributeValue("class" , "D");
    record->setRecordBoost(80);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1008);
    record->setSearchableAttributeValue(0, "Jimi Hendrix zzzzzz");
    record->setSearchableAttributeValue(1, "Little wing");
    record->setNonSearchableAttributeValue("citation", "8");
    record->setNonSearchableAttributeValue("price" , "3.34");
    record->setNonSearchableAttributeValue("class" , "E");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

bool checkResults(QueryResults *queryResults, vector<unsigned> *resultSet)

{
	if(queryResults->getNumberOfResults() != resultSet->size()) return false;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
    {
        vector<string> matchingKeywords;
        vector<unsigned> editDistances;

        queryResults->getMatchingKeywords(resultCounter, matchingKeywords);
        queryResults->getEditDistances(resultCounter, editDistances);


        LOG_REGION(0,
                cout<<"\nResult-("<<resultCounter<<") RecordId:"<<queryResults->getRecordId(resultCounter)<<"\tScore:"<<queryResults->getResultScoreString(resultCounter);
        );
        LOG_REGION(0,
                cout<<"\nMatching Keywords:"<<endl;
        );

        unsigned counter = 0;
        for(vector<string>::iterator iter = matchingKeywords.begin(); iter != matchingKeywords.end(); iter++, counter++ )
        {
            LOG_REGION(0,
                    cout<<"\t"<<*iter<<" "<<editDistances.at(counter)<<endl;
            );
        }

        if(resultSet->at(resultCounter) != atoi(queryResults->getRecordId(resultCounter).c_str()) )
        {
            return false;
        }

    }
    return true;
}
bool checkFacetedFilterResults(QueryResults * queryResults , std::map<std::string , std::vector<float> > * facetResults){
	std::map<std::string , std::vector<float> > & facetResultsForTest = queryResults->impl->facetResults;
	if(facetResultsForTest.size() != facetResults->size()){
		std::cout << "Facet results : " << std::endl;
		for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResultsForTest.begin() ;
				attrIter != facetResultsForTest.end() ; ++attrIter){
			std::cout << "Attribute : " <<  attrIter->first << std::endl;
			for(std::vector<float>::iterator aggregationResult = attrIter->second.begin() ;
					aggregationResult != attrIter->second.end() ; ++aggregationResult){
				std::cout << *aggregationResult << " ";
			}
			std::cout << "\n ======================================= \n" ;
		}
		std::cout << "Correct Facet results : " << std::endl;
		for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResults->begin() ;
				attrIter != facetResults->end() ; ++attrIter){
			std::cout << "Attribute : " <<  attrIter->first << std::endl;
			for(std::vector<float>::iterator aggregationResult = attrIter->second.begin() ;
					aggregationResult != attrIter->second.end() ; ++aggregationResult){
				std::cout << *aggregationResult << " ";
			}
			std::cout << "\n ============= \n" ;
		}
		return false;
	}
	for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResults->begin() ;
			attrIter != facetResults->end() ; ++attrIter){
		if(facetResultsForTest[attrIter->first] != (*facetResults)[attrIter->first]) {
			std::cout << "Facet results : " << std::endl;
			for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResultsForTest.begin() ;
					attrIter != facetResultsForTest.end() ; ++attrIter){
				std::cout << "Attribute : " <<  attrIter->first << std::endl;
				for(std::vector<float>::iterator aggregationResult = attrIter->second.begin() ;
						aggregationResult != attrIter->second.end() ; ++aggregationResult){
					std::cout << *aggregationResult << " ";
				}
				std::cout << "\n ============= \n" ;
			}
			return false;
		}
	}

	// test from the other direction
	for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResultsForTest.begin() ;
			attrIter != facetResultsForTest.end() ; ++attrIter){
		if(facetResultsForTest[attrIter->first] != (*facetResults)[attrIter->first]) {
			std::cout << "Facet results : " << std::endl;
			for(std::map<std::string , std::vector<float> >::iterator attrIter = facetResultsForTest.begin() ;
					attrIter != facetResultsForTest.end() ; ++attrIter){
				std::cout << "Attribute : " <<  attrIter->first << std::endl;
				for(std::vector<float>::iterator aggregationResult = attrIter->second.begin() ;
						aggregationResult != attrIter->second.end() ; ++aggregationResult){
					std::cout << *aggregationResult << " ";
				}
				std::cout << "\n ============= \n" ;
			}
			return false;
		}
	}

	return true;
}
void printResults(QueryResults *queryResults)
{
    cout<<"Results:"<<queryResults->getNumberOfResults()<<endl;
    for (unsigned resultCounter = 0;
            resultCounter < queryResults->getNumberOfResults(); resultCounter++ )
    {
        cout<<"Result-("<<resultCounter<<") RecordId:"<<queryResults->getRecordId(resultCounter)<<endl;
        cout << "[" << queryResults->getInternalRecordId(resultCounter) << "]" << endl;
        cout << "[" << queryResults->getInMemoryRecordString(resultCounter) << "]" << endl;
    }
}


void Test_Sort_Filter(IndexSearcherInternal *indexSearcherInternal){

    std::vector<unsigned> resultSet0 , resultSet01, resultSet02, resultSet03,  resultSet1, resultSet2, resultSetEmpty;

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




    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = {
            "pionn","fllio","shiii"
    };







    cout<<"\n***PREFIX FUZZY***\nQuery:";
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";


    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);


	ResultsPostProcessorPlan * plan = NULL;
	plan = new ResultsPostProcessorPlan();
	SortFilter * sortFilter = new SortFilter();
	// these two line must be changed by setting something like an expression folder in filter
	sortFilter->attributeName = "citation";
	sortFilter->order = srch2::instantsearch::Ascending;

	plan->addFilterToPlan(sortFilter);
	query->setPostProcessingPlan(plan);




    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);


    ASSERT(checkResults(queryResultsAfterFilter, &resultSet0));

    ////////////////////////////////////////////////////

	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	sortFilter = new SortFilter();
	// these two line must be changed by setting something like an expression folder in filter
	sortFilter->attributeName = "price";
	sortFilter->order = srch2::instantsearch::Ascending;

	plan->addFilterToPlan(sortFilter);
	query->setPostProcessingPlan(plan);




    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSet01));


    ////////////////////////////////////////////////////////


	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	sortFilter = new SortFilter();
	// these two line must be changed by setting something like an expression folder in filter
	sortFilter->attributeName = "class";
	sortFilter->order = srch2::instantsearch::Ascending;

	plan->addFilterToPlan(sortFilter);
	query->setPostProcessingPlan(plan);



    queryResults = new QueryResults(new QueryResultFactory(), indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSet02));

    ///////////////////////////////////////////////////////////


    // test with empty results
    type = PREFIX;
    cout<<"keywordwhichisnotinresults"<< "\n";


    Term *term1 = FuzzyTerm::create("keywordwhichisnotinresults", type, 1, 1, 2);
    query->add(term1);


	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	sortFilter = new SortFilter();
	// these two line must be changed by setting something like an expression folder in filter
	sortFilter->attributeName = "price";
	sortFilter->order = srch2::instantsearch::Ascending;

	plan->addFilterToPlan(sortFilter);
	query->setPostProcessingPlan(plan);




    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSetEmpty));



    delete query;
    delete queryResults;
//    delete queryResults1;
//    delete queryResults2;

}

void Test_Expression_Filter(IndexSearcherInternal *indexSearcherInternal){

    std::vector<unsigned> resultSet0 , resultSet01, resultSet02, resultSet03,  resultSet1, resultSet2, resultSetEmpty;

    // query = "pionn", citation < 7
    resultSet0.push_back(1003);
    resultSet0.push_back(1005);
    resultSet0.push_back(1006);

    // query = "pionn", price < 8
    resultSet01.push_back(1005);
    resultSet01.push_back(1006);
    resultSet01.push_back(1007);

    // query = "pionn", class < D
    resultSet02.push_back(1003);
    resultSet02.push_back(1005);
    resultSet02.push_back(1006);




    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[3] = {
            "pionn","fllio","shiii"
    };







    cout<<"\n***PREFIX FUZZY***\nQuery:";

    // query = "pionn", citation < 7
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);


	ResultsPostProcessorPlan * plan = NULL;
	plan = new ResultsPostProcessorPlan();
	NonSearchableAttributeExpressionFilter * expressionFilter = new NonSearchableAttributeExpressionFilter();
	// these two line must be changed by setting something like an expression folder in filter
	expressionFilter->attributeName = "citation";
	Score se7en;
	se7en.setScore((unsigned)7);
	expressionFilter->attributeValue = se7en;

	plan->addFilterToPlan(expressionFilter);
	query->setPostProcessingPlan(plan);




    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);


    ASSERT(checkResults(queryResultsAfterFilter, &resultSet0));

    ////////////////////////////////////////////////////
    // query = "pionn", price < 8
	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	expressionFilter = new NonSearchableAttributeExpressionFilter();
	// these two line must be changed by setting something like an expression folder in filter
	expressionFilter->attributeName = "price";
	Score eig8t;
	eig8t.setScore((float)8);
	expressionFilter->attributeValue = eig8t;

	plan->addFilterToPlan(expressionFilter);
	query->setPostProcessingPlan(plan);




    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSet01));


    ////////////////////////////////////////////////////////

    // query = "pionn", class < D
	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	expressionFilter = new NonSearchableAttributeExpressionFilter();
	// these two line must be changed by setting something like an expression folder in filter
	expressionFilter->attributeName = "class";
	Score D;
	D.setScore("D");
	expressionFilter->attributeValue = D;

	plan->addFilterToPlan(expressionFilter);
	query->setPostProcessingPlan(plan);



    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);

    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSet02));

    ///////////////////////////////////////////////////////////


    // test with empty results
    type = PREFIX;
    cout<<"keywordwhichisnotinresults"<< "\n";


    Term *term1 = FuzzyTerm::create("keywordwhichisnotinresults", type, 1, 1, 2);
    query->add(term1);


	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	expressionFilter = new NonSearchableAttributeExpressionFilter();
	// these two line must be changed by setting something like an expression folder in filter
	expressionFilter->attributeName = "citation";
	se7en.setScore((unsigned)7);
	expressionFilter->attributeValue = se7en;

	plan->addFilterToPlan(expressionFilter);
	query->setPostProcessingPlan(plan);




    queryResults = new QueryResults(new QueryResultFactory(),indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);

    ASSERT(checkResults(queryResultsAfterFilter, &resultSetEmpty));



    delete query;
    delete queryResults;
//    delete queryResults1;
//    delete queryResults2;

}


void Test_FacetedSearch_Filter(IndexSearcherInternal *indexSearcherInternal){

    std::vector<unsigned> resultSet0 , resultSet01, resultSet02, resultSet03,  resultSet1, resultSet2, resultSetEmpty;

    // query = "zzzzzzz", facet: class<A,B,C,D,E>
    resultSet0.push_back(1001);
    resultSet0.push_back(1002);
    resultSet0.push_back(1003);
    resultSet0.push_back(1004);
    resultSet0.push_back(1005);
    resultSet0.push_back(1006);
    resultSet0.push_back(1007);
    resultSet0.push_back(1008);



    std::map<std::string , std::vector<float> > facetResults; // A,B,C,D,E
    std::vector<float> classFacetResults ;
    classFacetResults.push_back(2);
    classFacetResults.push_back(2);
    classFacetResults.push_back(2);
    classFacetResults.push_back(1);
    classFacetResults.push_back(1);
    facetResults["class"] = classFacetResults;
    //////////////////////////////////

    std::map<std::string , std::vector<float> > facetResults1; // A,E
    std::vector<float> classFacetResults1 ;
    classFacetResults1.push_back(7);
    classFacetResults1.push_back(1);
    facetResults1["class"] = classFacetResults1;
    //////////////////////////////////


    std::map<std::string , std::vector<float> > facetResults2; //class : A,E & citation : 1,5
    std::vector<float> classFacetResults2 ;
    classFacetResults2.push_back(7);
    classFacetResults2.push_back(1);
    facetResults2["class"] = classFacetResults2;
    std::vector<float> citationFacetResults2 ;
    citationFacetResults2.push_back(4);
    citationFacetResults2.push_back(4);
    facetResults2["citation"] = citationFacetResults2;
    //////////////////////////////////

    std::map<std::string , std::vector<float> > facetResults3; //price : 5,7 & class : A,E & citation : 1
    std::vector<float> priceFacetResults3 ;
    priceFacetResults3.push_back(4); // 3.34,4.34,5.34,6.34
    priceFacetResults3.push_back(4); // 7.34,8.34,9.34,10.34
    facetResults3["price"] = priceFacetResults3;
    std::vector<float> classFacetResults3 ;
    classFacetResults3.push_back(7);
    classFacetResults3.push_back(1);
    facetResults3["class"] = classFacetResults3;
    std::vector<float> citationFacetResults3 ;
    citationFacetResults3.push_back(8);
    facetResults3["citation"] = citationFacetResults3;
    //////////////////////////////////


    int resultCount = 10;
    // create a query
    Query *query = new Query(srch2is::TopKQuery);
    string keywords[1] = {
            "zzzzzzz"
    };







    cout<<"\n***PREFIX FUZZY***\nQuery:";

    // query = "zzzzzzz",
    TermType type = PREFIX;
    cout<<keywords[0]<< "\n";
    Term *term0 = FuzzyTerm::create(keywords[0], type, 1, 1, 2);
    query->add(term0);

    // facet: class<A,B,C,D,E>
	ResultsPostProcessorPlan * plan = NULL;
	plan = new ResultsPostProcessorPlan();
	FacetedSearchFilter * facetFilter = new FacetedSearchFilter();
	Score lowerbounds[5];
	lowerbounds[0].setScore("A");
	lowerbounds[1].setScore("B");
	lowerbounds[2].setScore("C");
	lowerbounds[3].setScore("D");
	lowerbounds[4].setScore("E");
	std::vector<Score> lowerBounds;
	for(int s=0;s<5;s++){
		lowerBounds.push_back(lowerbounds[s]);
	}
	facetFilter->lowerBoundsOfCategories["class"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["class"] = srch2::instantsearch::Count;

	plan->addFilterToPlan(facetFilter);
	query->setPostProcessingPlan(plan);
	QueryResultFactory * factory  = new QueryResultFactory();
    QueryResults *queryResults = new QueryResults(factory,indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    QueryResults *queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
    ASSERT(checkFacetedFilterResults(queryResultsAfterFilter, &facetResults));

//    ASSERT(checkResults(queryResultsAfterFilter, &resultSet0));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;
    delete facetFilter;
    ////////////////////////////////////////////////////
    // facet: class<A,E> => A:7,E:1

	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	facetFilter = new FacetedSearchFilter();
	Score lowerbounds1[2];
	lowerbounds1[0].setScore("A");
	lowerbounds1[1].setScore("E");
	lowerBounds.clear();
	for(int s=0;s<2;s++){
		lowerBounds.push_back(lowerbounds1[s]);
	}
	facetFilter->lowerBoundsOfCategories["class"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["class"] = srch2::instantsearch::Count;

	plan->addFilterToPlan(facetFilter);
	query->setPostProcessingPlan(plan);
	factory  = new QueryResultFactory();
    queryResults = new QueryResults(factory,indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
    ASSERT(checkFacetedFilterResults(queryResultsAfterFilter, &facetResults1));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;
    delete facetFilter;

    //////////////////////////////////
    //class : A=7,E=1 & citation : 1=4,5=4
	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	facetFilter = new FacetedSearchFilter();
	//class : A,E
	Score lowerbounds2[2];
	lowerbounds2[0].setScore("A");
	lowerbounds2[1].setScore("E");
	lowerBounds.clear();
	for(int s=0;s<2;s++){
		lowerBounds.push_back(lowerbounds2[s]);
	}
	facetFilter->lowerBoundsOfCategories["class"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["class"] = srch2::instantsearch::Count;
	//citation : 1,5
	Score lowerbounds22[2];
	lowerbounds22[0].setScore((unsigned)1);
	lowerbounds22[1].setScore((unsigned)5);
	lowerBounds.clear();
	for(int s=0;s<2;s++){
		lowerBounds.push_back(lowerbounds22[s]);
	}
	facetFilter->lowerBoundsOfCategories["citation"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["citation"] = srch2::instantsearch::Count;

	plan->addFilterToPlan(facetFilter);
	query->setPostProcessingPlan(plan);
	factory  = new QueryResultFactory();
    queryResults = new QueryResults(factory,indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);


    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
    ASSERT(checkFacetedFilterResults(queryResultsAfterFilter, &facetResults2));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;
    delete facetFilter;


    /////////////////////////////////////////////////////////////
    //price : 7,4 & class : A,E & citation : 1
	plan = NULL;
	plan = new ResultsPostProcessorPlan();
	facetFilter = new FacetedSearchFilter();
	//price : 7,4
	Score lowerbounds3[2];
	lowerbounds3[0].setScore((float)5);
	lowerbounds3[1].setScore((float)7);
	lowerBounds.clear();
	for(int s=0;s<2;s++){
		lowerBounds.push_back(lowerbounds3[s]);
	}
	facetFilter->lowerBoundsOfCategories["price"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["price"] = srch2::instantsearch::Count;
	//class : A,E
	Score lowerbounds31[2];
	lowerbounds31[0].setScore("A");
	lowerbounds31[1].setScore("E");
	lowerBounds.clear();
	for(int s=0;s<2;s++){
		lowerBounds.push_back(lowerbounds31[s]);
	}
	facetFilter->lowerBoundsOfCategories["class"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["class"] = srch2::instantsearch::Count;
	//citation : 1
	Score lowerbounds32[1];
	lowerbounds32[0].setScore((unsigned)1);
	lowerBounds.clear();
	for(int s=0;s<1;s++){
		lowerBounds.push_back(lowerbounds32[s]);
	}
	facetFilter->lowerBoundsOfCategories["citation"] = lowerBounds;
	facetFilter->facetedSearchAggregationTypes["citation"] = srch2::instantsearch::Count;

	plan->addFilterToPlan(facetFilter);
	query->setPostProcessingPlan(plan);
	factory  = new QueryResultFactory();
    queryResults = new QueryResults(factory,indexSearcherInternal, query);
    indexSearcherInternal->search(query, queryResults, resultCount);
//    std::cout << "===============================================" << std::endl;
//    std::cout << "===============================================" << std::endl;
//    std::cout << "===============================================" << std::endl;
//    std::cout << "===============================================" << std::endl;

    queryResultsAfterFilter = applyFilter(queryResults,indexSearcherInternal,query);
    ASSERT(checkFacetedFilterResults(queryResultsAfterFilter, &facetResults3));
    delete plan;
    delete queryResults;
    delete queryResultsAfterFilter;
    delete factory;
    delete facetFilter;
}


void Searcher_Tests()
{
    addRecords();

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "");

    Indexer* indexer = Indexer::load(indexMetaData);

	IndexSearcherInternal *indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(IndexSearcher::create(indexer));



	std::cout << "test1" << std::endl;
	Test_Sort_Filter(indexSearcherInternal);
	std::cout << "test2" << std::endl;
	Test_Expression_Filter(indexSearcherInternal);
	std::cout << "test3" << std::endl;
	Test_FacetedSearch_Filter(indexSearcherInternal);

    delete indexer;
    delete indexSearcherInternal;
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }


    Searcher_Tests();

    cout << "FacetedSearchFilters Unit Tests: Passed\n";

    return 0;
}

